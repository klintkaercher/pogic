# pogic
`pogic` is both a TOML format for merging workspace-related documentation and an application to take in that TOML format and build the workspace.
You can look at the TOML format and see the expected information like sources and versions, as well as more technical information like additional flags you'd like passed and
extra documentation like `description` and `next-release`. This allows for changes in workspaces and the expected changes to be version controlled and queried.

This was mostly a project to work with downloading external dependencies with `cmake`, using `toml++`, and actually solving a problem of
having to bother someone to get answers on the new release that was kept in private notes.
