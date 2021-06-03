
use std::io::{stdin, BufRead};
use std::io::{stdout, Write};
use std::env;
use std::process::{exit, Command};
use std::process::Stdio;
use std::process::Child;

fn main() -> !
{
    // Ctrl+C handle: clear current line and start a new line
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

        while let Some(cmd) = cmds.next()
        {
            let mut args = cmd.split_whitespace();
            let prog = args.next();
            match prog
            {
                // Ctrl+D hanle: exit
                None =>
                {
                    println!("bye!");
                    exit(0);
                }
                Some (_prog) =>
                {
                }
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
                /*
                "pwd" =>
                {
                    println!("{}", env::current_dir().expect(dir_err).to_str().expect(dir_err));
                    prev_cmd = None;
                }
                */
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
                prog => {
                    let stdin = prev_cmd
                        .map_or(
                            Stdio::inherit(),
                            |output: Child| Stdio::from(output.stdout.unwrap())
                        );

                    let stdout = if cmds.peek().is_some() {
                        // there is another command piped behind this one
                        // prepare to send output to the next command
                        // 在这个命令后还有另一个命令，准备将其输出到下一个命令
                        Stdio::piped()
                    } else {
                        // there are no more commands piped behind this one
                        // send output to shell stdout
                        // 在发送输出到 shell 的 stdout 之后，就没有命令要执行了
                        Stdio::inherit()
                    };
                    
                    let output = Command::new(prog)
                        .args(args)
                        .stdin(stdin)
                        .stdout(stdout)
                        .spawn();
                    
                    //let output = Command::new(prog).args(args);
                    //let output = Command::new(prog).args(args).status().expect("Run program failed");

                    match output {
                        Ok(output) => { prev_cmd = Some(output); },
                        Err(e) => {
                            prev_cmd = None;
                            eprintln!("{}", e);
                        },
                    };

                    if let Some(ref mut final_cmd) = prev_cmd {
                        // block until the final command has finished
                        // 阻塞一直到命令执行完成
                        final_cmd.wait().unwrap();
                    }
                }
            }
        }
    }
}
