/*
c++: 23
package_definitions: true
deps:
    - pub.egorpugin.primitives.sw.main
*/

#include <primitives/hash.h>
#include <primitives/sw/main.h>
#include <bits/stdc++.h>
using namespace std;

size_t find_closing_curly_bracket(auto &&s) {
    auto it = s.begin();
    size_t n = 0;
    while (*it) {
        if (*it == '{') ++n;
        else if (*it == '}' && !--n)
            return distance(s.begin(), it);
        ++it;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    regex r_datafile{R"xxx(\\begin\{datafile\}\{(.*?)\})xxx"};
    regex r_input{R"xxx(\\\input\{(.*?)\})xxx"};
    regex r_include{R"xxx(\\\include\{(.*?)\})xxx"};
    smatch m;
    path fn = argv[1];
    ofstream ofile{argv[2]};
    ofstream dfile{argv[3]};
    dfile << "$(TMPFILE).tex :";
    auto lines = split_lines(read_file(fn), true);
    // preprocessor step
    for (auto it = lines.begin(); it != lines.end();) {
        auto &s = *it++;
        if (0) {
        } else if (regex_match(s, m, r_input) || regex_match(s, m, r_include)) {
            auto fn = m[1].str();
            auto lines2 = split_lines(read_file(fn), true);
            s = "%" + s;
            it = lines.insert(it, lines2.begin(), lines2.end());
            dfile << " " << fn;
            continue;
        }
    }
    // remove %commented lines
    for (auto it = lines.begin(); it != lines.end();) {
        if (it->starts_with('%'))
            it = lines.erase(it);
        else
            ++it;
    }
    int nline = 0;
    for (auto it = lines.begin(); it != lines.end();) {
        ++nline;
        auto &s = *it++;
        if (0) {
        } else if (s == "\\question") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\question{" << *it++ << "}" << "\n";
            ofile << "\\begin{enumerate}[label={\\asbuk*)}]" << "\n";
            while (*it != "")
                ofile << "\\item " << *it++ << "\n";
            ofile << "\\end{enumerate}" << "\n";
            ofile << "\n";
        } else if (s == "\\table") {
            static const regex r_multicolumn{R"xxx(\\\multicolumn\{(\d+)\})xxx"};
            static const regex r_label{R"xxx(\\\label\{(.*?)\})xxx"};
            bool bold_table_headers = false;
            bool nocaption = false;
            optional<string> caption;
            optional<string> label;
            int ncols = 0;
            int nrows = 0;
            Strings table;
            while (*it != "\\endtable") {
                auto &s = *it;
                if (0) {
                } else if (s == "\\boldheaders" || s == "\\жирныезаголовки") {
                    bold_table_headers = true;
                } else if (s.starts_with("\\rows")) {
                    nrows = std::stoi(s.substr(5));
                } else if (s.starts_with("\\nocaption")) {
                    nocaption = true;
                } else if (regex_match(s, m, r_label)) {
                    label = s;
                } else if (!caption && !nocaption) {
                    caption = s;
                } else if (s.starts_with("%")) {
                } else {
                    table.push_back(s);
                    ncols = max<int>(ncols, ranges::count(s, ';') + 1);
                }
                ++it;
            }
            ++it;

            if (nrows)
                table.resize(nrows);

            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{table}[ht!]" << "\n";
            if (label)
                ofile << *label << "\n";
            if (caption || label) {
                ofile << "\\caption{";
                if (caption)
                    ofile << *caption;
                if (label)
                    ofile << *label;
                ofile << "}" << "\n";
            }
            ofile << "\\centering" << "\n";
            ofile << "\\begin{tabularx}{\\textwidth}{|";
            auto headers = split_string(table[0], ";", true);
            for (int i = 0; i < ncols; ++i) {
                if (i < headers.size() && !headers[i].empty() && headers[i][0] == '{') {
                    ofile << headers[i].substr(1, find_closing_curly_bracket(headers[i]) - 1);
                } else {
                    ofile << "X";
                    //ofile << "p{" << (1.0 / ncols) << "\\textwidth}";
                }
                ofile << "|";
            }
            ofile << "}" << "\n";
            ofile << "\\hline" << "\n";
            for (int str = 0; auto &&s : table) {
                auto cols = split_string(s, ";", true);
                for (int i = 0, imulticol = 0; i < ncols; ++i, ++imulticol) {
                    if (i < cols.size()) {
                        bool multicolumn = regex_search(cols[i], m, r_multicolumn);
                        if (bold_table_headers && !multicolumn && str == 0)
                            ofile << "\\textbf{";
                        if (str == 0 && i < headers.size() && !headers[i].empty() && headers[i][0] == '{') {
                            ofile << cols[i].substr(find_closing_curly_bracket(cols[i]) + 1);
                        } else {
                            ofile << cols[i];
                        }
                        if (bold_table_headers && !multicolumn && str == 0)
                            ofile << "}";
                        if (multicolumn) {
                            imulticol += stoi(m[1].str()) - 1;
                        }
                    }
                    if (imulticol < ncols - 1)
                        ofile << " & ";
                }
                ofile << "\\\\\\hline\n";
                ++str;
            }
            ofile << "\\end{tabularx}" << "\n";
            ofile << "\\end{table}" << "\n";
            //ofile << "\n";
        } else if (regex_match(s, m, r_datafile)) {
            auto fn = m[1].str();
            string s;
            while (*it++ != "\\end{datafile}")
                s += s + "\n";
            write_file_if_different(fn, s);
        } else if (s == "\\items") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{itemize}" << "\n";
            while (*it != "")
                ofile << "\\item " << *it++ << "\n";
            ofile << "\\end{itemize}" << "\n";
            ofile << "\n";
        } else if (s == "\\enum") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{enumerate}" << "\n";
            while (*it != "")
                ofile << "\\item " << *it++ << "\n";
            ofile << "\\end{enumerate}" << "\n";
            ofile << "\n";
        // cpp (only with includes and inside main)
        // cppraw (no includes)
        // cppsw (with primitives.sw.main)
        } else if (s == "\\cppmain") {
            string s = R"xxx(
#include <bits/stdc++.h>
using namespace std;

int main(int argc, char *argv[]) {
            )xxx";
            while (*it != "\\endcppmain")
                s += *it++ + "\n";
            ++it;
            s += "}\n";
            const auto base = fs::temp_directory_path() / sha1(s);
            auto fn = path{base} += ".cpp";
            auto out = base;
            auto stdo = path{base} += ".out";
            if (!fs::exists(stdo)) {
                write_file_if_different(fn, s);
                {
                    string cmd = "g++ -std=c++23 " + fn.string() + " -o " + base.string();
                    if (system(cmd.c_str()))
                        throw SW_RUNTIME_ERROR("build error");
                }
                {
                    string cmd = base.string() + " > " + stdo.string();
                    if (system(cmd.c_str()))
                        throw SW_RUNTIME_ERROR("run error");
                }
            }
            auto lines2 = split_lines(read_file(stdo), true);
            it = lines.insert(it, lines2.begin(), lines2.end());
        } else {
            ofile << s << "\n";
        }
    }
    return 0;
}
