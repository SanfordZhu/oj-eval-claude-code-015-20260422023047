#include <bits/stdc++.h>
using namespace std;

// Bucketed append-only on-disk store to keep memory usage low.
// We use 16 bucket files based on hash(index). For find, we scan only the bucket
// and reconstruct the set for that index, then output sorted values.
// For insert/delete, we append records without keeping global state.

static const int BUCKETS = 16;
static const string BUCKET_PREFIX = "kv_"; // files: kv_00.dat ... kv_15.dat

struct Record {
    uint8_t op;     // 1=insert, 2=delete
    uint8_t klen;   // key length (<=64)
    // followed by klen bytes of key, then int32 value
};

static inline uint32_t h32(const string &s) {
    // FNV-1a 32-bit
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

static inline int bucket_of(const string &key) { return (int)(h32(key) % BUCKETS); }

static inline string bucket_path(int b) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%02d.dat", BUCKET_PREFIX.c_str(), b);
    return string(buf);
}

static bool write_one(ofstream &ofs, uint8_t op, const string &key, int value) {
    uint8_t klen = (uint8_t)min<size_t>(key.size(), 64u);
    ofs.write(reinterpret_cast<const char*>(&op), 1);
    ofs.write(reinterpret_cast<const char*>(&klen), 1);
    ofs.write(key.data(), klen);
    int32_t v = value;
    ofs.write(reinterpret_cast<const char*>(&v), 4);
    return ofs.good();
}

static bool read_one(ifstream &ifs, uint8_t &op, string &key, int &value) {
    uint8_t klen;
    if (!ifs.read(reinterpret_cast<char*>(&op), 1)) return false;
    if (!ifs.read(reinterpret_cast<char*>(&klen), 1)) return false;
    key.resize(klen);
    if (!ifs.read(&key[0], klen)) return false;
    int32_t v;
    if (!ifs.read(reinterpret_cast<char*>(&v), 4)) return false;
    value = (int)v;
    return true;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Open bucket files for append (created if not exist)
    array<ofstream, BUCKETS> outs;
    for (int b = 0; b < BUCKETS; ++b) {
        outs[b].open(bucket_path(b), ios::binary | ios::app);
    }

    int n; if (!(cin >> n)) return 0;
    string cmd, key; int value;

    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            cin >> key >> value;
            int b = bucket_of(key);
            write_one(outs[b], 1, key, value);
        } else if (cmd == "delete") {
            cin >> key >> value;
            int b = bucket_of(key);
            write_one(outs[b], 2, key, value);
        } else if (cmd == "find") {
            cin >> key;
            int b = bucket_of(key);
            // Ensure recent writes are visible
            outs[b].flush();
            ifstream ifs(bucket_path(b), ios::binary);
            if (!ifs.good()) {
                cout << "null\n";
                continue;
            }
            uint8_t op; string k; int v;
            unordered_set<int> present;
            present.reserve(64);
            while (read_one(ifs, op, k, v)) {
                if (k == key) {
                    if (op == 1) present.insert(v);
                    else if (op == 2) present.erase(v);
                }
            }
            if (present.empty()) {
                cout << "null\n";
            } else {
                vector<int> vals(present.begin(), present.end());
                sort(vals.begin(), vals.end());
                for (size_t i2 = 0; i2 < vals.size(); ++i2) {
                    if (i2) cout << ' ';
                    cout << vals[i2];
                }
                cout << '\n';
            }
        }
    }

    return 0;
}
