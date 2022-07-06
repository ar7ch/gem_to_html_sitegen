#pragma once

/**
 * @file parser.hpp
 * @author Artem Bulgakov
 *
 * @brief parsing .gmi to .html and auxiliary functions
 */

namespace parser {
    using namespace std;
    const string WHITESPACE = " \n\r\t\f\v";

    /**
     * @brief Removes whitespace and formatting (tab, CR, LF, ..) characters from the left end of the string.
     * @param s input string
     * @return left-trimmed string
     */
    string ltrim(const string &s)
    {
        size_t start = s.find_first_not_of(WHITESPACE);
        return (start == string::npos) ? "" : s.substr(start);
    }

    /**
     * @brief Removes whitespace and formatting (tab, CR, LF, ..) characters from the right end of the string.
     * @param s input string
     * @return right-trimmed string
     */
    string rtrim(const string &s)
    {
        size_t end = s.find_last_not_of(WHITESPACE);
        return (end == string::npos) ? "" : s.substr(0, end + 1);
    }

    /**
     * @brief Removes whitespace and formatting (tab, CR, LF, ..) characters from both ends of the string.
     * @param s input string
     * @return trimmed string
     */
    string trim(const string &s) {
        return rtrim(ltrim(s));
    }

    /**
     * Markup tags go here
     */
    struct gem {
        static const string list,
            preformat,
            header,
            quote,
            hyperlink;
    };
    const string gem::list = "*",
            gem::preformat = "```",
            gem::header = "#",
            gem::quote = ">",
            gem::hyperlink = "=>";

    struct html {
        static const string list_start,
            list_end,
            list_item_start,
            list_item_end,
            preformat_start,
            preformat_end,
            quote_start,
            quote_end,
            header_start,
            header_end,
            hyperlink_start,
            hyperlink_end;
    };
    const string html::list_start = "<ul>",
            html::list_end = "</ul>",
            html::list_item_start = "<li>",
            html::list_item_end = "</li>",
            html::quote_start = "<blockquote>",
            html::quote_end = "</blockquote>",
            html::preformat_start="<pre>",
            html::preformat_end="</pre>",
            html::header_start="<h{}>",
            html::header_end="</h{}>",
            html::hyperlink_start="<a href=\"{}\">",
            html::hyperlink_end="</a>";

    /**
     * @brief Extremely simple format function to substitute strings (unless std::format from C++20 is unavailable in g++) (only one substitution accepted).
     * @param fmt format string "{}" substring marks for substitution position
     * @param arg string to be substituted with
     * @return a substituted string.
     */
    string format(string fmt, string arg) {
        static string fmt_str = "{}";
        return fmt.replace(fmt.find("{}"), fmt_str.size(), arg);
    }

    /**
     * Converts a Gemtext file into HTML by parsing the corresponding tags.
     * @param in stream for input .gmi file
     * @param out stream for output .html ашду
     */
    void convert_gmi_to_html(ifstream& in, ofstream& out) {
        bool in_list = false, in_pref = false; // flags to indicate putting end tags
        string input_line;
        while(getline(in, input_line)) {
            string line = trim(input_line); // copy elision will be performed, no need for a move

            // get a space-delimited prefix using string stream
            string line_prefix; istringstream iss(line); iss >> line_prefix;

            // get the rest of the string, trimming the leading whitespaces
            string line_rest = ltrim(line.substr(line_prefix.size()));

            // if in preformat block and it is not ending now, just print the plain line
            if(in_pref and line_prefix != gem::preformat) {
                out << input_line << endl;
                continue;
            }
            // if we had a list elements before and now we don't have any, put list ending tag
            if(in_list and line_prefix != gem::list) {
                out << html::list_end << endl;
                in_list = false;
            }

            // parse tags
            if(line_prefix[0] == gem::header[0]) { // consider first element to cover multiple header levels (they all start with #)
                size_t h_level = line_prefix.size();
                string hl = to_string(h_level);
                out << format(html::header_start, hl) << line_rest << format(html::header_end, hl) << endl;
            }
            else if(line_prefix == gem::quote) {
                out << html::quote_start << line_rest << html::quote_end << endl;
            }
            else if(line_prefix == gem::hyperlink) {
                istringstream iss_href(line_rest);
                //get the link as the space-delimited substring
                string href; iss_href >> href;
                // cut the rest of the string as the link text
                string link_name = trim(line_rest.substr(href.size()));
                out << format(html::hyperlink_start, href) << link_name << html::hyperlink_end << endl;
            }
            else if(line_prefix == gem::list){
                if(not in_list) { // put list starting tag
                    in_list = true;
                    out << html::list_start << endl;
                }
                // put list item tags
                out << html::list_item_start << line_rest << html::list_item_end << endl;
            }
            else if(line_prefix == gem::preformat) {
                if(not in_pref) {
                    out << html::preformat_start << endl;
                    in_pref = true;
                } else {
                    out << html::preformat_end << endl;
                    in_pref = false;
                }
                // static analyzer complains of unreachable code when flipping the flag: in_pref = !in_pref; so let me leave it like this
            } else {
                // matching gemtext tag not found: skipping
                out << input_line << endl;
            }
        }
        if (in_list) { // if the last line is list item; it wasn't closed in a loop; close manually
            in_list = false;
            out << html::list_end << endl;
        }
    }
}