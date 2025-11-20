/*
    Similar to reference code but added:
        - Progress bar display in terminal
        - 8KB buffer comparison rather than byte-by-byte
        - command line flags for min/max file size, sorting (oldest, newest, shortest path), and help
        - computing hash only for files with > 1 occurrence of that file size

*/

#include <iostream>
#include <cstring>
#include <filesystem>
#include <unordered_map>
#include <unordered_set> //for unique file sizes hash performance
#include <vector>
#include <fstream>
#include <optional>
#include <string>
#include <iomanip> //for formatting progress terminal output
#include <chrono> //for formatting file last write time
#include <algorithm>// for sorting output

using namespace std;
namespace fs = std::filesystem;

//const uint64_t MAX_FILE_SIZE = 1024 * 1024 * 1000;
enum class SortPolicy { NONE, NEWEST, OLDEST, SHORTEST_PATH };

// ---------------------------------------------
//  FileKey: Composite key (file_size + hash)
// ---------------------------------------------
struct FileKey {
    uint64_t size;
    uint64_t hash;

    bool operator==(const FileKey& o) const {
        return size == o.size && hash == o.hash;
    }
};

struct FileKeyHash {
    size_t operator()(const FileKey& k) const noexcept {
        // 64-bit mix (similar to boost::hash_combine)
        uint64_t h = k.hash;
        h ^= k.size + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return static_cast<size_t>(h);
    }
};

// ---------------------------------------------
//  File hashing: buffered + DJB2 + xxhash-style mix
// ---------------------------------------------
class FileHash {
public:
    static uint64_t fast_hash(const string& path)
    {
        ifstream file(path, ios::binary);
        if (!file.is_open())
            return 0;

        uint64_t hash = 5381;
        char buffer[4096];

        while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
            size_t n = file.gcount();
            for (size_t i = 0; i < n; i++) {
                hash = ((hash << 5) + hash) + static_cast<unsigned char>(buffer[i]); // DJB2
            }
        }

        // Mix to reduce collisions
        hash ^= (hash >> 33);
        hash *= 0xff51afd7ed558ccdULL;
        hash ^= (hash >> 33);
        hash *= 0xc4ceb9fe1a85ec53ULL;
        hash ^= (hash >> 33);

        return hash;
    }
};

// ---------------------------------------------
//  File discovery
// ---------------------------------------------
class FileDiscovery {
public:
    static vector<string> find(const vector<string>& paths)
    {
        vector<string> out;
        out.reserve(4096);

        for (const auto& p : paths) {
            try {
                for (const auto& entry : fs::recursive_directory_iterator(p)) {
                    if (entry.is_regular_file())
                        out.push_back(entry.path().string());
                }
            }
            catch (const exception& e) {
                cerr << "Error scanning " << p << ": " << e.what() << endl;
            }
        }
        return out;
    }
};

// ---------------------------------------------
//  ht: A clean wrapper over unordered_map
//      ht<Key> : vector<string>
// ---------------------------------------------
template <typename Key, typename Hash = std::hash<Key>>
class ht : public unordered_map<Key, vector<string>, Hash>
{
public:

    // Insert file path under the key
    void insert_value(const Key& k, const string& v)
    {
        (*this)[k].push_back(v);
    }

    
};

// ---------------------------------------------
//  Byte-by-byte exact comparison
//      UPDATED from vector<pair<s,s>> to vector<vector<string>> for groups
// ---------------------------------------------
class FileMatcher {
public:
    static void exact_compare(const vector<string>& paths, vector<vector<string>>& groups)
    {
        vector<bool> assigned(paths.size(), false);

        for (size_t i = 0; i < paths.size(); i++) {
            if (assigned[i]) continue;

            vector<string> group;
            group.push_back(paths[i]);
            assigned[i] = true;

            for (size_t j = i + 1; j < paths.size(); j++) {
                if (assigned[j]) continue;

                if (fs::file_size(paths[i]) != fs::file_size(paths[j]))
                    continue;

                ifstream f1(paths[i], ios::binary);
                ifstream f2(paths[j], ios::binary);
                if (!f1.is_open() || !f2.is_open())
                    continue;

                istreambuf_iterator<char> b1(f1), e1;
                istreambuf_iterator<char> b2(f2), e2;

                if (equal(b1, e1, b2)) {
                    group.push_back(paths[j]);
                    assigned[j] = true;
                }
            }

            // Only add groups with actual duplicates
            if (group.size() > 1) {
                groups.push_back(group);
            }
        }
    }

    //------------------------
    // buffered exact compare
    // AI suggested this method since comparing files with min-size of 100MB was taking minutes
    //------------------------
    static void buffer_exact_compare(const vector<string>& paths, vector<vector<string>>& groups) {
        vector<bool> assigned(paths.size(), false);

        for (size_t i = 0; i < paths.size(); i++) {
            if (assigned[i]) continue;

            vector<string> group;
            group.push_back(paths[i]);
            assigned[i] = true;

            for (size_t j = i + 1; j < paths.size(); j++) {
                if (assigned[j]) continue;

                if (fs::file_size(paths[i]) != fs::file_size(paths[j]))
                    continue;

                ifstream f1(paths[i], ios::binary);
                ifstream f2(paths[j], ios::binary);
                if (!f1.is_open() || !f2.is_open())
                    continue;

                // buffered comparison (faster than iterators)
                const size_t BUFFER_SIZE = 8192; //reading 8kb instead of byte by byte
                char buf1[BUFFER_SIZE], buf2[BUFFER_SIZE];
                bool match = true;
                while (match) {
                    //read buffer
                    f1.read(buf1, BUFFER_SIZE);
                    f2.read(buf2, BUFFER_SIZE);
                    
                    streamsize n1 = f1.gcount(); //counts how many bytes were read
                    streamsize n2 = f2.gcount();
                    
                    //if # bytes aren't equal OR compare n1 bytes from the two buffers
                    if (n1 != n2 || memcmp(buf1, buf2, n1) != 0) {
                        match = false;
                        break;
                    }
                    
                    //if EOF and match still = true, they're a match
                    if (f1.eof() && f2.eof()) break;
                }

                if (match) {
                    group.push_back(paths[j]);
                    assigned[j] = true;
                }
            }
            //add groups with duplicates
            if (group.size() > 1) {
                groups.push_back(group);
            }
        }
    }

    static vector<vector<string>> find_matches(const vector<string>& paths, int minFileSize, int maxFileSize)
    {

        uint64_t minBytes = minFileSize * 1024ULL * 1024ULL;
        uint64_t maxBytes = maxFileSize * 1024ULL * 1024ULL;
        
        // Discover files
        auto files = FileDiscovery::find(paths);
        vector<string> filteredFiles;
        //filter by size
        for(const auto& f: files){
            uint64_t fSize = fs::file_size(f);
            if(minFileSize > 0 && fSize < minBytes) continue;
            if(maxFileSize > 0 && fSize > maxBytes) continue;
            filteredFiles.push_back(f);
        }

        // We already have total # of files:
        int total = filteredFiles.size();
        int current = 0;
        float percent;
        ht<FileKey, FileKeyHash> table;

        // map of fileSize => quant
        unordered_map<uint64_t, int> fileSizes;
        for(const auto& f : filteredFiles){
            uint64_t fSize = fs::file_size(f);
            if(fileSizes[fSize] == 0){
                fileSizes[fSize] = 1;
            }
            else{
                fileSizes[fSize] = fileSizes[fSize]+1;
            }
        }

        // Hash each file ONLY for good fileSize
        for (const auto& f : filteredFiles) {
            //Progress bar
            cout << fixed << setprecision(2); // for rounding
            
            current++;
            percent = (float)current/(float)total;

            percent = percent * 100;
            int percentLoop = percent/10;
            
            cout << "\rHASHING PROGRESS: [";
            for(int i = 0; i < percentLoop; i++){
                cout << "█";
            }
            for(int i = percentLoop; i < 10; i++){
                cout << "░";
            }
            cout << "] ";
            cout << percent << " %";

            uint64_t sz = fs::file_size(f);

            // hash ONLY if the fileSize in our map has > 1 count
            if(fileSizes[sz] > 1){
                uint64_t h  = FileHash::fast_hash(f);
                FileKey key{ sz, h };
                table.insert_value(key, f);
            }
        }
  
        cout << endl;
        // Compare files inside each bucket
        vector<vector<string>> groups;

        int buckets= 0;
        int totalBuckets = 0;
        // count buckets
        for (auto& [key, vec] : table) {
            if (vec.size() > 1) totalBuckets++;
        }

        // Now process with progress
        for (auto& [key, vec] : table) {
            if (vec.size() > 1) {
                buckets++;
                percent = (float)buckets/(float)totalBuckets;
                percent = percent * 100;
                int percentLoop = percent/10;
                
                cout << "\rPROCESSING PROGRESS: [";
                for(int i = 0; i < percentLoop; i++){
                    cout << "█";
                }
                for(int i = percentLoop; i < 10; i++){
                    cout << "░";
                }
                cout << "] ";
                cout << percent << " %";
                buffer_exact_compare(vec, groups);
                //exact_compare(vec, groups);
            }
        }
        return groups;
    }
};

// ----------------------------
// for printing file write time. converts the time
// ----------------------------
string getFileWriteTime(string file){
    auto ftime = fs::last_write_time(file);
    auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + chrono::system_clock::now()
    );
    time_t cftime = chrono::system_clock::to_time_t(sctp);
    return ctime(&cftime);
}

// ---------------------------------------------
//  Main
// ---------------------------------------------
int main(int argc, char* argv[])
{
    SortPolicy policy = SortPolicy::NEWEST;
    int maxFileSize = 0;
    int minFileSize = 0;
    vector<string> paths;

    // CLI flag processing
    for(int i = 1; i < argc; i++){
        string arg = argv[i];
        if(arg == "-s" || arg == "--sort"){
            if(i+1 > argc){
                cerr << "Error: --sort requries a policy. See -h or --help for info.\n";
                exit(1);
            }
            string policyString = argv[i+1];
            i++;
            if(policyString == "newest"){
                policy = SortPolicy::NEWEST;
            }
            else if(policyString == "oldest"){
                policy = SortPolicy::OLDEST;
            }
            else if(policyString == "shortest_path"){
                policy = SortPolicy::SHORTEST_PATH;
            }
            else{
                policy = SortPolicy::NONE;
            }
        }
        else if(arg == "--max-size"){
            if(i+1 > argc){
                cerr << "Error: --max-size requries a value. See -h or --help for info.\n";
                exit(1);
            }
            try{
                maxFileSize = stoull(argv[i+1]);
                i++;
            }
            catch (const invalid_argument& e) {
                cerr << "Error: Invalid size value '" << argv[i + 1] << "'\n";
                exit(1);
            } catch (const out_of_range& e) {
                cerr << "Error: Size value out of range\n";
                exit(1);
            }
        }
        else if(arg == "--min-size"){
            if(i+1 > argc){
                cerr << "Error: --min-size requries a value. See -h or --help for info.\n";
                exit(1);
            }
            try{
                minFileSize = stoull(argv[i+1]);
                i++;
            }
            catch (const invalid_argument& e) {
                cerr << "Error: Invalid size value '" << argv[i + 1] << "'\n";
                exit(1);
            } catch (const out_of_range& e) {
                cerr << "Error: Size value out of range\n";
                exit(1);
            }
        }
        //ai helped format this
        else if(arg == "--help" || arg == "-h"){
            cout << R"(
                FileMatcher - Find duplicate files

                USAGE:
                ./fileMatcher [OPTIONS] <directory1> [directory2 ...]

                OPTIONS:
                -h, --help              Show this help message
                -s, --sort <policy>     Sort duplicate groups by policy:
                                            newest        - Newest files first
                                            oldest        - Oldest files first
                                            shortest_path - Shortest paths first
                --min-size <size in MB> The minimum file size to scan for in MB.
                --max-size <size in MB> The maximum file size to scan for in MB.

                EXAMPLES:
                ./fileMatcher /home/user/Documents
                ./fileMatcher --sort newest /path1 /path2
                )" << endl;
            exit(0);
        }
        else{
            paths.push_back(arg);
        }
    }

    //vector<string> paths = { "." };
    auto matches = FileMatcher::find_matches(paths, minFileSize, maxFileSize);

    //sort every group based on policy
    for (auto& group : matches) {
        switch (policy) {
            case SortPolicy::NEWEST:
                sort(group.begin(), group.end(), [](const string& a, const string& b) {
                    return getFileWriteTime(a) > getFileWriteTime(b);
                });
                break;
            case SortPolicy::OLDEST:
                sort(group.begin(), group.end(), [](const string& a, const string& b) {
                    return getFileWriteTime(a) < getFileWriteTime(b);
                });
                break;
            case SortPolicy::SHORTEST_PATH:
                sort(group.begin(), group.end(), [](const string& a, const string& b) {
                    return a.length() < b.length();
                });
                break;
            case SortPolicy::NONE:
                break;
        }
    }

    // ... print groups ...
    cout << "\n--- EXACT MATCHES ---\n";
    int groupNum = 1;
    for (auto& group : matches){
        cout << "GROUP " << groupNum++ << " (" << group.size() << " files):\n";
        for (auto& file : group) {
            cout << "\t" << file << "\t" << (float)fs::file_size(file)/(1024*1024)<<"mb...\t..."<< getFileWriteTime(file)<<"\n";
        }
        cout << "\n";
    }

    cout << "FOUND " << matches.size() << " MATCHES\n";
}
