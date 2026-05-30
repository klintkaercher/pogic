//! The main function you'll want to look at is `try_from` for Workspace.
//! `pogic` has a pretty clean execution. It finds and parses a workspace file, then iterates over
//! the repos, building the git clone command for each and then executing it.
//! Sections are:
//! --> DATA_STRUCTURES
//! --> IMPL
//! --> UTILITY
//! --> TEST

// --> DATA_STRUCTURES START

/// Repo is the basic building block. Our app does nothing if we don't have `Repo`s.
#[derive(Clone, Debug, Default, PartialEq, PartialOrd)]
pub struct Repo {
    name: Option<String>,
    url: String,
    path: Option<String>,
    version: Option<String>,
    description: Option<String>,
    next_version: Option<String>,
    next_version_description: Option<String>,
    extra_flags: Vec<String>,
}

/// Our Workspace is the holder of the Repos and has our global information.
/// It is our root table in the toml file and predominantly empty in rosinstalls and .repos files.
#[derive(Clone, Debug, Default, PartialEq, PartialOrd)]
pub struct Workspace {
    name: Option<String>,
    version: Option<String>,
    next_version: Option<String>,
    next_version_description: Option<String>,
    description: Option<String>,
    repos: Vec<Repo>,
}

/// Generic error for describing various bad states of our program.
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub enum Error {
    Parse(String),
    FileRead,
}

/// Holds command line arguments.
#[derive(Clone, Debug, Default, PartialEq, PartialOrd)]
pub struct Config {
    pub dry_run: bool,
    pub file_name: Option<String>,
}

// --> DATA_STRUCTURES END

// --> IMPL START

impl Repo {
    /// This is for a naive implementation where we're just building command args
    /// and then going to Command. After the parsing, this function does the most work.
    fn get_args(&self) -> Vec<String> {
        let mut output = vec!["clone".to_string()];
        if let Some(version) = self.version.clone() {
            output.push("--branch".to_string());
            output.push(version);
        }

        for it in self.extra_flags.iter() {
            if it.contains(" ") {
                output.push(format!("\"{}\"", it))
            } else {
                output.push(it.clone())
            };
        }

        output.push(self.url.clone());

        if let Some(path) = self.path.clone() {
            output.push(path);
        }

        output
    }
}

impl Workspace {
    /// Iterates over the [`Repo`]s, constructs the [`std::process::Command`]s, and runs them.
    pub fn run(&self, config: &Config) {
        for it in self.repos.iter() {
            println!("Running `{}`", it.name.clone().unwrap_or(it.url.clone()));

            if config.dry_run {
                println!("{:?}", it);
                continue;
            }

            // --> COMMAND_EXECUTION START
            let output = std::process::Command::new("git")
                .args(it.get_args())
                .output();

            if output.is_err() {
                eprintln!("Had an error with `{}`", it.url);
                eprintln!("{:?}", output);
            }
            // --> COMMAND_EXECUTION END
        }
    }
}

impl TryFrom<&str> for Workspace {
    type Error = Error;

    /// The main working function. Actual execution of the clone commands is done in `main`.
    /// If you're looking to add keys to `pogic.toml`
    fn try_from(file_name: &str) -> Result<Self, Self::Error> {
        let Ok(contents) = std::fs::read_to_string(file_name) else {
            return Err(Error::FileRead);
        };
        'toml: {
            use toml::Value;

            let Ok(root_table): Result<toml::Table, _> = toml::from_str(&contents) else {
                break 'toml Err(Error::Parse(format!("File `{file_name}` is not toml.")));
            };

            let mut output = Workspace {
                name: root_table
                    .get("name")
                    .and_then(Value::as_str)
                    .map(String::from),
                version: root_table
                    .get("version")
                    .and_then(Value::as_str)
                    .map(String::from),
                next_version: root_table
                    .get("next-version")
                    .and_then(Value::as_str)
                    .map(String::from),
                next_version_description: root_table
                    .get("next-version-description")
                    .and_then(Value::as_str)
                    .map(String::from),
                description: root_table
                    .get("description")
                    .and_then(Value::as_str)
                    .map(String::from),
                repos: Vec::new(),
            };

            if let Some(repos) = root_table.get("repos").and_then(Value::as_array) {
                for it in repos.iter() {
                    if let Some(url) = it.as_str() {
                        output.repos.push(Repo {
                            url: url.to_string(),
                            ..Default::default()
                        });
                    } else if let Some(tab) = it.as_table() {
                        output.repos.push(("", tab).into());
                    }
                }
            }

            for (k, v) in root_table.iter() {
                if let Some(table) = v.as_table() {
                    output.repos.push((k.as_str(), table).into());
                }
            }
            Ok(output)
        }
        .or('rosinstall: {
            use serde_yaml::Value;

            let Ok(yaml): Result<Value, _> = serde_yaml::from_str(&contents) else {
                break 'rosinstall Err(Error::Parse(format!("File `{file_name}` is not yaml.")));
            };
            let mut output = Workspace::default();

            let Some(seq) = yaml.as_sequence() else {
                break 'rosinstall Err(Error::Parse(format!(
                    "File `{file_name}` is not a rosinstall yaml."
                )));
            };

            for it in seq.iter().flat_map(Value::as_mapping) {
                if let Some(elem) = it.get("git").and_then(Value::as_mapping) {
                    output.repos.push(Repo {
                        name: None,
                        url: elem.get("uri").unwrap().as_str().unwrap().to_string(),
                        path: Some(
                            elem.get("local-name")
                                .unwrap()
                                .as_str()
                                .unwrap()
                                .to_string(),
                        ),
                        version: elem
                            .get("version")
                            .and_then(Value::as_str)
                            .map(String::from),
                        ..Default::default()
                    })
                } else {
                    eprintln!("Don't know how to handle `{it:?}`");
                }
            }
            Ok(output)
        })
        .or('repos: {
            use serde_yaml::Value;

            let Ok(yaml): Result<Value, _> = dbg!(serde_yaml::from_str(&contents)) else {
                break 'repos Err(Error::Parse(format!(
                    "File `{file_name}` is not yaml for vcstool."
                )));
            };

            let Some(yaml) = yaml.as_mapping() else {
                break 'repos Err(Error::Parse(format!(
                    "File `{file_name}` is not repo yaml."
                )));
            };

            let mut output = Workspace::default();

            let Some(yaml) = yaml
                .get("repositories")
                .and_then(serde_yaml::Value::as_mapping)
            else {
                break 'repos Err(Error::Parse("".to_string()));
            };

            for it in yaml.keys() {
                let elem = yaml.get(it).and_then(serde_yaml::Value::as_mapping);
                if elem
                    .and_then(|elem| elem.get("type"))
                    .and_then(serde_yaml::Value::as_str)
                    != Some("git")
                {
                    eprintln!("Don't know how to handle `{:?}`", yaml.get("type"));
                }

                output.repos.push(Repo {
                    name: None,
                    url: elem
                        .and_then(|e| e.get("url"))
                        .and_then(serde_yaml::Value::as_str)
                        .unwrap()
                        .to_string(),
                    path: it.as_str().map(String::from),
                    version: elem
                        .and_then(|e| e.get("version"))
                        .and_then(serde_yaml::Value::as_str)
                        .map(String::from),
                    ..Default::default()
                })
            }

            Ok(output)
        })
    }
}

impl From<(&str, &toml::Table)> for Repo {
    fn from((key, table): (&str, &toml::Table)) -> Self {
        use toml::Value;

        Self {
            name: if key.is_empty() {
                None
            } else {
                Some(key.to_string())
            },
            url: table
                .get("url")
                .expect("`url` field is a required string.")
                .as_str()
                .expect("Problem with string conversion.")
                .to_string(),
            path: table.get("path").and_then(Value::as_str).map(String::from),
            version: table
                .get("version")
                .and_then(Value::as_str)
                .map(String::from),
            next_version: table
                .get("next-version")
                .and_then(Value::as_str)
                .map(String::from),
            next_version_description: table
                .get("next-version-description")
                .and_then(Value::as_str)
                .map(String::from),
            description: table
                .get("description")
                .and_then(Value::as_str)
                .map(String::from),
            extra_flags: table
                .get("extra-flags")
                .and_then(Value::as_array)
                .map(|it| {
                    it.iter().fold(Vec::new(), |mut acc, elem| {
                        acc.push(
                            elem.as_str()
                                .expect("`extra_flags` field expects an array of strings.")
                                .to_string(),
                        );
                        acc
                    })
                })
                .unwrap_or_default(),
        }
    }
}
// --> IMPL END

// --> UTIL START
pub fn parse_args(args: Vec<String>) -> Result<Config, String> {
    let mut dry_run = false;
    let mut file_name = None;
    for arg in args {
        if arg == "--dry-run" {
            dry_run = true;
        } else {
            file_name = Some(arg);
        }
    }

    Ok(Config { dry_run, file_name })
}
// --> UTIL END

// --> TEST START
#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn rosinstall_conversion() {
        let workspace = dbg!(Workspace::try_from("sample.rosinstall")).unwrap();
        assert_eq!(workspace.repos.len(), 4);
    }

    #[test]
    fn vcs_repos_conversion() {
        let workspace = Workspace::try_from("sample.repos").unwrap();

        assert_eq!(workspace.repos.len(), 4);
    }
}
// --> TEST END
