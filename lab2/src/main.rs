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
    ctrlc::set_handler
    (||{
        let dir_err = "Getting current dir failed";
        print!
        (
            "{}:{}# ",
            env::var_os("LOGNAME").unwrap().to_str().unwrap(), 
            env::current_dir().expect(dir_err).to_str().expect(dir_err)
        );
        stdout().flush().unwrap();
    }).expect("set handle error");

    loop
    {
        let dir_err = "Getting current dir failed";
        print!
        (
            "{}:{}# ",
            env::var_os("LOGNAME").unwrap().to_str().unwrap(), 
            env::current_dir().expect(dir_err).to_str().expect(dir_err)
        );
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
            let prog = args.next();
            
            // Ctrl+D handler: exit
            if !prog.is_some()
            {
                println!("bye!");
                exit(0);
            }

            let prog = prog.unwrap();

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
                    let mut delayed_stdin = 0;
                    let mut delayed_stdin_string = "";

                    let mut stdout = if cmds.peek().is_some() {
                        // there is another command piped behind this one
                        // prepare to send output to the next command
                        Stdio::piped()
                    } else {
                        // there are no more commands piped behind this one
                        // send output to shell stdout
                        Stdio::inherit()
                    };

                    // "<" support
                    let mut file_redirect_split: Vec<&str> = cmd.split(" < ").collect();
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        let filename = file_redirect_split[1];
                        let inputfile = File::open(filename).expect("Cannot open file");
                        stdin = Stdio::from(inputfile);
                    }

                    // "<<" support
                    file_redirect_split = cmd.split(" << ").collect();
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        delayed_stdin = 1;
                        delayed_stdin_string = file_redirect_split[1];
                        stdin = Stdio::piped();
                    }

                    // "<<<" support
                    file_redirect_split = cmd.split(" <<< ").collect();
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        delayed_stdin = 2;
                        delayed_stdin_string = file_redirect_split[1];
                        stdin = Stdio::piped();
                    }

                    // ">" support
                    file_redirect_split = cmd.split(" > ").collect();
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        let filename = file_redirect_split[1];
                        let outputfile = File::create(filename).expect("Cannot open file");
                        stdout = Stdio::from(outputfile);
                    }
                    
                    // ">>" support
                    file_redirect_split = cmd.split(" >> ").collect();
                    if file_redirect_split.len() == 2
                    {
                        cmd = file_redirect_split[0];
                        let filename = file_redirect_split[1];
                        let outputfile = std::fs::OpenOptions::new().append(true).open(filename).expect("Cannot open file");
                        stdout = Stdio::from(outputfile);
                    }

                    // reparse args and prog
                    args = cmd.split_whitespace();
                    prog = args.next().unwrap();
                    
                    let mut output = Command::new(prog)
                        .args(args)
                        .stdin(stdin)
                        .stdout(stdout)
                        .spawn()
                        .expect("execution error");
                    
                    if delayed_stdin == 1 // "<<"
                    {
                        let mut delayed_stdin_child = output.stdin.take().expect("Failed to open stdin");
                        let mut all_input = String::new();
                        loop
                        {
                            print!("> ");
                            std::io::stdout().flush().unwrap();

                            let mut input = String::new();
                            for line_res in std::io::stdin().lock().lines()
                            {
                                let line = line_res.expect("Read a line from stdin failed");
                                input = line;
                                break;
                            }
                            if input == delayed_stdin_string
                            {
                                break;
                            }
                            all_input += &input;
                            all_input += "\n";
                        }
                        delayed_stdin_child.write_all(all_input.as_bytes()).expect("Failed to write to stdin");
                    }

                    if delayed_stdin == 2 // "<<<"
                    {
                        let mut delayed_stdin_child = output.stdin.take().expect("Failed to open stdin");
                        delayed_stdin_child.write_all(delayed_stdin_string.as_bytes()).expect("Failed to write to stdin");
                    }

                    prev_cmd = Some(output);

                    if let Some(ref mut final_cmd) = prev_cmd {
                        // block until the final command has finished
                        final_cmd.wait().unwrap();
                    }
                }
            }
        }
    }
}
