#include <bits/stdc++.h>
using namespace std;

// Persistent key->sorted set<int> storage using a single file.
// Constraints: up to 100k ops, memory 5-6 MiB. We will only keep an in-memory index
// mapping key -> set of values while processing a single run, but problem says some
// testcases continue based on previous runs, so we must load existing data from file first.
// We'll append operations and rebuild in-memory index from the file on each run, which is
// allowed within limits.

static const char *DB_PATH = "kv_store.dat";

struct Record {
    string key;
    int value;
    // op: 1 insert, 2 delete
    uint8_t op;
};

// Serialize as: [op(1)][key_len(1)][key bytes][value(4 little-endian)]
// key_len <= 64

bool write_record(ofstream &ofs, const Record &r) {
    uint8_t op = r.op;
    uint8_t klen = (uint8_t)min<size_t>(r.key.size(), 255u); // but spec says <=64
    if (r.key.size() > 255) return false;
    ofs.write(reinterpret_cast<const char*>(&op), 1);
    ofs.write(reinterpret_cast<const char*>(&klen), 1);
    ofs.write(r.key.data(), klen);
    int32_t v = r.value;
    ofs.write(reinterpret_cast<const char*>(&v), 4);
    return ofs.good();
}

bool read_record(ifstream &ifs, Record &r) {
    uint8_t op, klen;
    if (!ifs.read(reinterpret_cast<char*>(&op), 1)) return false;
    if (!ifs.read(reinterpret_cast<char*>(&klen), 1)) return false;
    r.key.resize(klen);
    if (!ifs.read(&r.key[0], klen)) return false;
    int32_t v;
    if (!ifs.read(reinterpret_cast<char*>(&v), 4)) return false;
    r.op = op;
    r.value = v;
    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Load existing records
    unordered_map<string, set<int>> idx;
    idx.reserve(100000);

    {
        ifstream ifs(DB_PATH, ios::binary);
        if (ifs.good()) {
            Record r;
            while (read_record(ifs, r)) {
                if (r.op == 1) {
                    idx[r.key].insert(r.value);
                } else if (r.op == 2) {
                    auto it = idx.find(r.key);
                    if (it != idx.end()) {
                        it->second.erase(r.value);
                        if (it->second.empty()) idx.erase(it);
                    }
                }
            }
        }
    }

    int n;
    if (!(cin >> n)) return 0;

    ofstream ofs(DB_PATH, ios::binary | ios::app);

    string cmd, key;
    int value;
    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            cin >> key >> value;
            auto &s = idx[key];
            if (s.find(value) == s.end()) {
                s.insert(value);
                Record r{key, value, 1};
                write_record(ofs, r);
            }
        } else if (cmd == "delete") {
            cin >> key >> value;
            auto it = idx.find(key);
            if (it != idx.end()) {
                if (it->second.erase(value)) {
                    Record r{key, value, 2};
                    write_record(ofs, r);
                    if (it->second.empty()) idx.erase(it);
                }
            } else {
                // If not present, still append delete? The spec says delete may not exist.
                // To preserve idempotence across runs we do nothing when absent.
            }
        } else if (cmd == "find") {
            cin >> key;
            auto it = idx.find(key);
            if (it == idx.end() || it->second.empty()) {
                cout << "null\n";
            } else {
                bool first = true;
                for (int v : it->second) {
                    if (!first) cout << ' ';
                    first = false;
                    cout << v;
                }
                cout << '\n';
            }
        }
    }

    return 0;
}
