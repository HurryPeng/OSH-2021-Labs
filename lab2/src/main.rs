use std::io::{stdin, BufRead};
use std::io::{stdout, Write};
use std::env;
use std::process::{exit, Command};

fn main() -> ! {
    loop {
        let dir_err = "Getting current dir failed";
        print!("{}# ", env::current_dir().expect(dir_err).to_str().expect(dir_err));
        stdout().flush();

        let mut cmd = String::new();
        for line_res in stdin().lock().lines() {
            let line = line_res.expect("Read a line from stdin failed");
            cmd = line;
            break;
        }
        let mut args = cmd.split_whitespace();
        let prog = args.next();

        match prog {
            None => panic!("Not program input"),
            Some(prog) => {
                match prog {
                    "cd" => {
                        let dir = args.next().expect("No enough args to set current dir");
                        env::set_current_dir(dir).expect("Changing current dir failed");
                    }
                    "pwd" => {
                        let err = "Getting current dir failed";
                        println!("{}", env::current_dir().expect(err).to_str().expect(err));
                    }
                    "export" => {
                        for arg in args {
                            let mut assign = arg.split("=");
                            let name = assign.next().expect("No variable name");
                            let value = assign.next().expect("No variable value");
                            env::set_var(name, value);
                        }
                    }
                    "exit" => {
                        exit(0);
                    }
                    _ => {
                        Command::new(prog).args(args).status().expect("Run program failed");
                    }
                }
            }
        }
    }
}
