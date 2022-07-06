/**
 * @file main.cpp
 * @author Artem Bulgakov
 *
 * @brief main code: parse input arguments, start file processing jobs
 */

#include <iostream>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <fstream>
#include <thread>
#include "BS_thread_pool.hpp"
#include "parser.hpp"

using namespace std;
namespace fs = filesystem;

BS::thread_pool tpool;
BS::synced_stream sync_out;
bool verbose = false;

/**
 * @brief A container class to represent transfer job proccessed by a worker thread
 * Stores:
 * - path to the existing file i_path
 * - path to the file to be copied/parsed o_path
 */
class FileEntity{
    public:
    fs::path i_path;
    fs::path o_path;

    FileEntity(fs::path& in_path, fs::path& out_path) : i_path(in_path), o_path(out_path) {}
    FileEntity(const fs::directory_entry& direntry, const FileEntity& parent) {
        this->i_path = parent.i_path / direntry.path().filename();
        this->o_path = parent.o_path / direntry.path().filename();
    }
    FileEntity(const FileEntity& other) = default;

    FileEntity(FileEntity&& other) noexcept : i_path(move(other.i_path)), o_path(move(other.o_path)) {}

    string extension() const {
        return this->i_path.extension().string();
    }

    bool is_directory() const {
        return fs::is_directory(this->i_path);
    }

    bool is_file() const {
        return fs::is_regular_file(this->i_path);
    }
};

void handle_job(const FileEntity& fe);
void handle_directory(const FileEntity& dir);
void handle_file(const FileEntity& file);


/**
 * @brief Processes directory files;
 * creates a new directory specified by dir.o_path if needed;
 * iterates through elements in directory specified by dir.i_path and submits new jobs to the thread pool.
 * @param dir directory to be considered.
 */
void handle_directory(const FileEntity& dir) {
    try {
        if (not exists(dir.o_path)) {
            fs::create_directory(dir.o_path);
            if (verbose) { sync_out.println("created directory " + dir.o_path.string()); }
        } else {
            if (verbose) { sync_out.println("directory " + dir.o_path.string() + " already exists, skipping"); }
        }
        for (auto &dir_entry: fs::directory_iterator(dir.i_path)) {
            if (dir_entry.is_directory() or dir_entry.is_regular_file()) {
                FileEntity child_fe(dir_entry, dir);
                tpool.push_task(handle_job, child_fe);
            }
        }
    } catch(fs::filesystem_error& err) {
        cout << err.what() << endl;
    }
}

/**
 * @brief Processes file jobs (copy / parse).
 * - File specified by output path already exists and shouldn't be parsed: skip
 * - File not exists and shouldn't be parsed: copy
 * - File should be parsed: parse, write results to file specified by o_path;
 * --- if the file with parsed extension already exists, it will be overwritten.
 * @param file considered file
 */
void handle_file(const FileEntity& file) {
    try{
        ostringstream oss;
        if(file.extension() == ".gmi") {
            FileEntity child_file(file);
            child_file.o_path.replace_extension(fs::path(".html"));
            ifstream file_in; file_in.open(child_file.i_path, ios::in);
            ofstream file_out; file_out.open(child_file.o_path, ios::out);
            if(not file_in.is_open()) {
                throw fs::filesystem_error("Unable to open for reading", file.i_path, error_code());
            }
            if(not file_out.is_open()){
                throw fs::filesystem_error("Unable to open for writing", file.o_path, error_code());
            }
            parser::convert_gmi_to_html(file_in, file_out);
            if(verbose) {oss << "parsed " << fs::canonical(file.i_path).string() << " -> " << fs::canonical(child_file.o_path).string() << endl; sync_out.print(oss.str()); }
        } else if (not fs::exists(file.o_path)){
            fs::copy_file(file.i_path, file.o_path);
            if(verbose) {oss << "copied " << fs::canonical(file.i_path).string() << " -> " << fs::canonical(file.o_path).string() << endl; sync_out.print(oss.str());}
        } else {
            if(verbose) {oss << fs::canonical(file.o_path).string() << " already exists, skipping" << endl; sync_out.print(oss.str());}
        }
    } catch(fs::filesystem_error& err) {
        cout << err.what() << endl;
    }
}

/**
 * @brief Dispatches a job to corresponding action.
 * @param fe file entity to be considered.
 */
void handle_job(const FileEntity& fe) {
    if (fe.is_directory()) {
        handle_directory(fe);
    }
    else if (fe.is_file()) {
        handle_file(fe);
    }
    else {
        assert(false);
    }
}


/***
 * Parses commandline arguments.
 * @param argc argument count (from main)
 * @param argv array of c-strings (from main)
 * @return a 3-tuple: path to input dir, path to output dir, verbosity flag
 */
tuple<string, string, bool> parse_args(int argc, char * argv[]) {
    ostringstream iss;
    iss << "usage: " << argv[0] << " <input_dir> <output_dir>" << " [-v]";

    bool verbose = false;
    if(argc < 3 or argc > 4) {
        throw std::runtime_error(iss.str()); }
    string arg1(argv[1]), arg2(argv[2]);
    if (argc == 4) {
        // for consistency, I could construct a std::string and compare it...
        // I'll leave c-string here to show you I can use functions from standard C library
        if(strcmp(argv[3], "-v") == 0) {
            verbose = true;
        } else {
            throw std::runtime_error(iss.str());
        }
    }
    return std::tuple<string, string, bool>{arg1, arg2, verbose};
}

int main(int argc, char * argv[]) {
    try {
        // fetch arguments
        auto [in_str, out_str, verbose] = parse_args(argc, argv);
        // set global verbosity flag
        ::verbose = verbose;
        // from path arguments, create std::filesystem objects; input_dir must exist
        fs::path input_dir = fs::path(in_str);
        input_dir = fs::canonical(fs::path(in_str)); // builtin existence check
        fs::path output_dir = fs::absolute(fs::path(out_str));
        // create initial job
        FileEntity base_dir(input_dir, output_dir);
        // push the job into the thread pool
        if(verbose) { cout << "Starting with " << thread::hardware_concurrency() << " threads" << endl; }
        tpool.push_task(handle_job, base_dir);
        tpool.wait_for_tasks();
    } catch (std::exception& err) {
        cerr << err.what() << endl;
        return EXIT_FAILURE;
    }
    if(verbose) { cout << "Done" << endl; }
    return EXIT_SUCCESS;
}