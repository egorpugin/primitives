#include <fstream>
#include <sstream>
#include <vector>

extern "C" void process_response_file(int *in_argc, char ***in_argv) {
    if (!(*in_argc == 2 && (*in_argv)[1] && (*in_argv)[1][0] == '@')) {
        return;
    }
    std::ifstream ifs(&(*in_argv)[1][1]);
    //std::ostringstream oss;
    //oss << ifs.rdbuf();
    //auto entireFile = oss.str();

    // options
    bool strip_quotes = true; // for perl
    //bool strip_single_quote = false; // dont edit single quote line '"'

    static std::vector<std::string> args;
    static std::vector<char*> argv;

    argv.emplace_back(*in_argv[0]);
    std::string str;
    while (std::getline(ifs, str)) {
        auto &s = args.emplace_back(str);
        if (strip_quotes && s.size() > 1 && s[0] == '\"' && s.back() == '\"') {
            s = s.substr(1, s.size() - 2);
        }
    }
    for (auto &&s : args) {
        argv.emplace_back(s.data());
    }
    argv.emplace_back(nullptr);

    *in_argc = args.size();
    *in_argv = argv.data();
}
