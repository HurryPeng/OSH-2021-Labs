use std::io::{stdin, BufRead};
use std::io::{stdout, Write};
use std::env;
use std::process::{exit, Command};
use std::process::Stdio;
use std::process::Child;
use std::fs::File;

fn main() -> !
{
    // Ctrl+C handler: clear current line and start a new line
    ctrlc::set_handler(||{
        let dir_err = "Getting current dir failed";
        print!("\n{}# ", env::current_dir().expect(dir_err).to_str().expect(dir_err));
        stdout().flush().unwrap();
    }).expect("set handle error");

    loop
    {
        let dir_err = "Getting current dir failed";
        print!("{}# ", env::current_dir().expect(dir_err).to_str().expect(dir_err));
        stdout().flush().unwrap();
        
        let mut input = String::new();
        for line_res in stdin().lock().lines()
        {
            let line = line_res.expect("Read a line from stdin failed");
            input = line;
            break;
        }

        let mut cmds = input.trim().split(" | ").peekable();
        let mut prev_cmd = None;

        while let Some(mut cmd) = cmds.next()
        {
            let mut args = cmd.split_whitespace();
            let mut prog = args.next();
            
            // Ctrl+D handler: exit
            if !prog.is_some()
            {
                println!("bye!");
                exit(0);
            }

            let mut prog = prog.unwrap();

            match prog
            {
                "cd" =>
                {
                    let dir = args.next().expect("No enough args to set current dir");
                    env::set_current_dir(dir).expect("Changing current dir failed");
                    prev_cmd = None;
                }
                "export" =>
                {
                    for arg in args {
                        let mut assign = arg.split("=");
                        let name = assign.next().expect("No variable name");
                        let value = assign.next().expect("No variable value");
                        env::set_var(name, value);
                    }
                    prev_cmd = None;
                }
                "exit" =>
                {
                    println!("bye!");
                    exit(0);
                }
                mut prog => {
                    let mut stdin = prev_cmd
                        .map_or(
                            Stdio::inherit(),
                            |output: Child| Stdio::from(output.stdout.unwrap())
                        );

                    let mut stdout = if cmds.peek().is_some() {
                        // there is another command piped behind this one
                        // prepare to send output to the next command
                        Stdio::piped()
                    } else {
                        // there are no more commands piped behind this one
                        // send output to shell stdout
                        Stdio::inherit()
                    };

                    let file_redirect_split: Vec<&str> = cmd.split(" < ").collect();
                    //println!("debug: {:?}", cmd);
                    //println!("debug: {:?}", file_redirect_split);
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        let filename = file_redirect_split[1];
                        let inputfile = File::open(filename).unwrap();
                        stdin = Stdio::from(inputfile);
                    }

                    args = cmd.split_whitespace();
                    prog = args.next().unwrap();
                    
                    let output = Command::new(prog)
                        .args(args)
                        .stdin(stdin)
                        .stdout(stdout)
                        .spawn();
                
                    match output {
                        Ok(output) => { prev_cmd = Some(output); },
                        Err(e) => {
                            prev_cmd = None;
                            eprintln!("{}", e);
                        },
                    };

                    if let Some(ref mut final_cmd) = prev_cmd {
                        // block until the final command has finished
                        final_cmd.wait().unwrap();
                    }
                }
            }
        }
    }
}
