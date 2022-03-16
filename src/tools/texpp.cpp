/*
c++: 23
package_definitions: true
deps:
    - pub.egorpugin.primitives.sw.main
*/

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
    regex r_multicolumn{R"xxx(\\\multicolumn\{(\d+)\})xxx"};
    regex r_input{R"xxx(\\\input\{(.*?)\})xxx"};
    regex r_include{R"xxx(\\\include\{(.*?)\})xxx"};
    smatch m;
    path fn = argv[1];
    ofstream ofile{argv[2]};
    ofstream dfile{argv[3]};
    dfile << "$(TMPFILE).tex :";
    auto lines = split_lines(read_file(fn), true);
    int nline = 0;
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        ++nline;
        if (0) {
        } else if (regex_match(*it, m, r_input) || regex_match(*it, m, r_include)) {
            auto fn = m[1].str();
            auto lines2 = split_lines(read_file(fn), true);
            lines.insert(++it, lines2.begin(), lines2.end());
            --it;
            dfile << " " << fn;
            continue;
        } else if (*it == "\\question") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\question{" << *++it << "}" << "\n";
            ofile << "\\begin{enumerate}[label={\\asbuk*)}]" << "\n";
            while (*++it != "")
                ofile << "\\item " << *it << "\n";
            ofile << "\\end{enumerate}" << "\n";
            ofile << "\n";
        } else if (*it == "\\table") {
            bool bold_table_headers = false;
            string caption;
            int ncols = 0;
            Strings table;
            while (*++it != "\\endtable") {
                auto &&s = *it;
                if (s == "\\boldheaders" || s == "\\жирныезаголовки") {
                    bold_table_headers = true;
                } else if (caption.empty()) {
                    caption = s;
                } else {
                    table.push_back(s);
                    ncols = max<int>(ncols, ranges::count(s, ';') + 1);
                }
            }
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{table}[ht!]" << "\n";
            ofile << "\\caption{" << caption << "}" << "\n";
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
                for (int i = 0; i < ncols; ++i) {
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
                            i += stoi(m[1].str()) - 1;
                        }
                    }
                    if (i < ncols - 1)
                        ofile << " & ";
                }
                ofile << "\\\\\\hline\n";
                ++str;
            }
            ofile << "\\end{tabularx}" << "\n";
            ofile << "\\end{table}" << "\n";
            //ofile << "\n";
        } else if (regex_match(*it, m, r_datafile)) {
            auto fn = m[1].str();
            string s;
            while (*++it != "\\end{datafile}")
                s += *it + "\n";
            write_file_if_different(fn, s);
        } else if (*it == "\\items") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{itemize}" << "\n";
            while (*++it != "")
                ofile << "\\item " << *it << "\n";
            ofile << "\\end{itemize}" << "\n";
            ofile << "\n";
        } else if (*it == "\\enum") {
            ofile << "%#line " << nline << " " << fn << "\n";
            ofile << "\\begin{enumerate}" << "\n";
            while (*++it != "")
                ofile << "\\item " << *it << "\n";
            ofile << "\\end{enumerate}" << "\n";
            ofile << "\n";
        } else {
            ofile << *it << "\n";
        }
    }
    return 0;
}
