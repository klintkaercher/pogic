#include "khelper.hpp"
#include <toml++/toml.h>

using namespace khelper;

struct Repo {
    String name                     = "";
    String source                   = "";
    String path                     = "";
    Option<String> version          = {};
    Option<String> next_version     = {};
    Option<Vec<String>> extra_flags = {};
    Option<String> description      = {};

    // This is for a naive implementation where we're just building command
    // strings and then going to `exec` them.
    // TODO check this for putting the right version in there.
    // TODO: In the future, we should look to use the git2 library.
    auto get_command() const -> String {
        String args = "";
        if (this->extra_flags) {
            for (const auto &it : this->extra_flags.value()) {
                String tmp = (find(" ", it)) ? quote_string(it) : it;
                args       = format("{} {} ", args, tmp);
            }
        }

        return format("git clone {} {} {} {}",
                      (this->version) ? "--branch " + this->version.value()
                                      : "",
                      args, this->source, this->path);
    }
};

auto operator<<(std::ostream &os, const Repo &rhs) -> std::ostream & {
    os << format(
        "Repo { name: {}, path: {}, version: {}, source: {}, next_version: "
        "{}, extra_flags: {}, description: {} }",
        rhs.name, rhs.path, rhs.version, rhs.source, rhs.next_version,
        rhs.extra_flags, rhs.description, rhs.version, rhs.source,
        rhs.next_version, rhs.extra_flags, rhs.description);
    return os;
}

auto to_string(const Repo &input) -> String {
    std::ostringstream oss;
    oss << input;
    return oss.str();
}

struct Workspace {
    String name                 = "";
    String version              = "";
    Option<String> next_version = {};
    Option<String> description  = {};
    Vec<Repo> repos             = {};
};

auto operator<<(std::ostream &os, const Workspace &rhs) -> std::ostream & {
    os << format(
        "Workpace { name: {}, version: {}, next_version: {}, description: "
        "{}, repos: {} }",
        rhs.name, rhs.version, rhs.next_version, rhs.description, rhs.repos);
    return os;
}

auto to_string(const Workspace &input) -> String {
    std::ostringstream oss;
    oss << input;
    return oss.str();
}

auto exec(const char *cmd) -> String {
    std::array<char, 128> buffer;
    std::string result;

    // Open pipe to file
    FILE *pipe = popen(cmd, "r");
    if (!pipe) { throw std::runtime_error("popen() failed!"); }

    // Read till end of process
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    pclose(pipe);
    return result;
}

int main(int argc, const char **argv) {
    if (argc == 1) {
        // TODO:  Need to make this actually better.
        std::cerr << "You need to give me a file name.\n";
        return 1;
    }

    toml::table root_table = toml::parse_file(argv[1]);

    Workspace workspace
        = {.name         = expect(root_table["name"].value<String>(),
                                  "`name` field is a required string."),
           .version      = expect(root_table["version"].value<String>(),
                                  "`version` field is a required string."),
           .next_version = root_table["next-version"].value<String>(),
           .description  = root_table["description"].value<String>()};

    for (const auto &[k, v] : root_table) {
        if (auto table_ptr = v.as_table(); table_ptr) {
            // Process the repo table.
            Repo repo
                = {.name    = String{k.str()},
                   .source  = expect((*table_ptr)["source"].value<String>(),
                                     "`source` field is a required string."),
                   .path    = expect((*table_ptr)["path"].value<String>(),
                                     "`path` field is a required string."),
                   .version = (*table_ptr)["version"].value<String>(),
                   .next_version = (*table_ptr)["next-version"].value<String>(),
                   .description  = (*table_ptr)["description"].value<String>()};

            if (auto it = (*table_ptr)["extra-flags"].as_array(); it) {
                Vec<String> tmp = {};
                for (const auto &elem : *it) {
                    tmp.push_back(expect(
                        elem.value<String>(),
                        "`extra_flags` field expects an array of strings."));
                }
                repo.extra_flags = tmp;
            }
            workspace.repos.push_back(repo);
        }
    }

    for (const auto &it : workspace.repos) {
        println("Running `{}`", it.get_command());
        exec(it.get_command().c_str());
    }

    return 0;
}
