# pogic
`pogic` is both a TOML format for merging workspace-related documentation and an application to take in that TOML format and build the workspace.
You can look at the TOML format and see the expected information like urls and versions,
as well as more technical information like additional flags you'd like passed and
extra documentation like `description` and `next-release`.
This allows for changes in workspaces and the expected changes to be version controlled and queried.

## Pogic Files
`pogic` works on pogic files, rosinstalls for `wstool`, and .repos files for `vcstool`.
If you use `pogic.txt` or `pogic.toml` in the folder that `pogic` is called in, `pogic` will detect them and use them.
`pogic.txt` and `pogic.toml` are special names for the default workspace files. If both are present, a warning will be emitted.

The `<FILE>` argument in the following commands is for non-standard name files.with `pogic.toml` only being considered if both are present.
You will get warnings if both `pogic.toml` and `pogic.txt` are present.

## Safety
`pogic` will execute arbitrary code that is in the `pre-build-cmd` and `post-build-cmd` fields in the pogic file.
This is for maximum flexbility, but does pose a security concern.
You can check what will be executed with `pogic check-cmd`.
`pogic` does not recurse currently, so you only need to validate _your_ pogic file.
The intent for `pogic` at this time is a single file that represents and builds the final workspace.

## Interface
#### Build
"Go through this pogic file and give me all the repos."
We believe this will be the most common use case and want to make that the easiest case.

We want to cover the gamut of use cases from "Hey, I have literally copy/pasted a bunch of repos into a file. Please build this workspace" to
"I'm getting ready to drop version 7.2.4 of my codebase and want to confirm the new pogic file works and generate the release notes".
```
pogic
pogic <FILE>
pogic b
pogic build
pogic b <FILE>
pogic build <FILE>
```

The `b` and `build` variants are for when people forget the default `pogic` behavior.

#### Scrape
"Oh no, I have a folder with a lot of repos in it somehow! How can get I get a `pogic.toml` from this?
Please alert me if I have any dirty repos, but please still log them regardless."

We expect this to be the second most common use case for `pogic`. Creating `pogic.toml` files and then building from them.
Here we have `<FILE>` be an output file name instead of an input.
```
pogic scrape
pogic scrape <FILE>
```

#### Diff
"Oh no, which repos have deviated from my pogic file? Which repos are in my workspace, but not in my pogic file?
Give me details with a flag."
This is meant to act like a `wstool status`, but really the difference is the focus.
```
pogic d
pogic diff
pogic d --verbose
pogic diff --verbose
pogic d -v
pogic diff -v
```

#### Translation Commands
"I have this old rosinstall. I want to migrate over to `pogic`."
```
pogic txt-to-toml
pogic txt-to-toml <FILE>
pogic rosinstall-to-toml <FILE>
pogic repos-to-toml <FILE>
```

#### Workspace Management
"Git pull all the git repos in our pogic file from this current, root folder and give me a flag for all repos."
This will not change version to match the `pogic` file. This will just `git pull` what is there.
```
pogic pull
pogic pull --all
```

"I've changed my workspace somehow and I just want to switch/checkout back to what
is described in my pogic file."
This command will pull. It's meant to `reset` the workspace. Look at `refresh` to pull as well.
```
pogic reset
pogic reset <FILE>
```

"My pogic file is all `main` branches and tags. I'm currently on some feature branches on some repos.
I want to be back on all those described `main` branches and tags, and please update the `main` branches as well."
```
pogic refresh
pogic refresh <FILE>
```

#### Miscellaneous
"Update my `pogic` file to what I have currently in my workspace. Alert me for dirty repos.
If you find a new repo, ask me to add to my pogic file with defaulting to the 'yes' option.
Have a flag where it just says yes for all new repos."
```
pogic update
pogic update --yes
```

"Tell me which of my repos have planned updates. What are their current versions? What is their planned versions?
With a flag, give me their next-version-description. With other flags, filter only major/minor/patch version changes."
For `--[major|minor|patch]`, `pogic` assumes semantic versioning like `major.minor.patch`.
In cases where the version doesn't match that scheme, those flags will not work as intended.
```
pogic updating --verbose
pogic updating -v
pogic updating -v --major
pogic updating --verbose <FILE>
pogic updating -v --patch <FILE>
```
These commands will output something like the following:
```
repo1         0.1.0 => 0.2.0
repo2         0.2.0 => 0.2.1
```

"I'd like some markdown release notes with the old versions, new versions, and reasons."
`<FILE>` is the output file for the release notes, with `release_notes.md` being the default.
This command overwrites `<FILE>` or `release_notes.md`
```
pogic release-notes
pogic release-notes <FILE>
```

"Please save me from typing so many `rm -rf` commands, but only for the repos that are in this pogic file."
```
pogic nuke
pogic nuke <FILE>
```
