//! Sections are:
//! --> MAIN_WORK
//! --> PARSE_PROCESS_ARGS

use pogic::*;

fn main() {
    // --> PARSE_PROCESS_ARGS START
    let args = std::env::args().collect::<Vec<String>>();

    let mut args = parse_args(args).unwrap_or_else(|e| {
        eprintln!("{e}");
        std::process::exit(1);
    });

    if let Some(ref file_name) = args.file_name
        && !std::fs::exists(&file_name).unwrap_or_default()
    {
        eprintln!("Could not find `{file_name}` or it was unaccessible.");
        std::process::exit(1);
    } else if std::fs::exists("pogic.toml").unwrap_or_default() {
        args.file_name = Some("pogic.toml".to_string());
    } else if std::fs::exists("pogic.txt").unwrap_or_default() {
        args.file_name = Some("pogic.txt".to_string());
    }
    // --> PARSE_PROCESS_ARGS END

    // --> MAIN_WORK START
    let file_name = args
        .file_name
        .as_ref()
        .expect("A pogic file name has not been populated at a point where it should have been.");

    match Workspace::try_from(file_name.as_str()) {
        Ok(workspace) => workspace.run(&args),
        Err(Error::FileRead) => eprintln!("Could not properly parse `{file_name}`"),
        Err(Error::Parse(e)) => eprintln!("Could not properly parse `{file_name}`: {e}"),
    }
    // --> MAIN_WORK END
}
