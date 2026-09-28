//-----------------------------------------------------------------------------
// BinToScenario -- command line
//
//   BinToScenario <input.bin> [output.scp] [--entity-type N] [--no-compress]
//
// Reads a recorded HOST->IG CIGI stream and writes a script for the native
// CIGI Host Emulator.  See BinToScenario.h for what it translates and why, and
// README.md for the bench procedure it fits into.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "BinToScenario.h"

namespace {

void usage(const char* prog) {
    std::printf(
        "Usage: %s <input.bin> [output.scp] [options]\n"
        "\n"
        "Rewrite a recorded HOST->IG CIGI stream (.bin) as a CIGI Host Emulator\n"
        "script (.scp), so the same scenario can be run on the native emulator\n"
        "instead of replayed through the .bin players.\n"
        "\n"
        "Options:\n"
        "  --entity-type <n>  emulator entity type for the ADD_ENTITY lines\n"
        "                     (default 118, the f18c in the emulator's own\n"
        "                     Default\\Entities.def)\n"
        "  --no-compress      one FRAME per datagram, instead of folding runs of\n"
        "                     unchanged frames into FRAME_WAIT\n"
        "  -h, --help         this text\n"
        "\n"
        "With no output path the input's extension becomes .scp.\n",
        prog);
}

bool readWholeFile(const std::string& path, std::vector<uint8_t>& out) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return false;
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (n < 0) { std::fclose(f); return false; }
    out.resize(static_cast<size_t>(n));
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return got == out.size();
}

bool writeWholeFile(const std::string& path, const std::string& text) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") != 0 || !f) return false;
    const size_t put = text.empty() ? 0 : std::fwrite(text.data(), 1, text.size(), f);
    std::fclose(f);
    return put == text.size();
}

} // namespace

int main(int argc, char** argv) {
    std::string in, out;
    bool haveOut = false;
    binscp::Options opt;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            usage(argv[0]);
            return 0;
        } else if (a == "--no-compress") {
            opt.frameWaitCompression = false;
        } else if (a == "--entity-type") {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "--entity-type needs a value\n");
                return 2;
            }
            opt.entityType = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 10));
        } else if (!a.empty() && a[0] == '-') {
            std::fprintf(stderr, "unknown option: %s\n", a.c_str());
            return 2;
        } else if (in.empty()) {
            in = a;
        } else if (!haveOut) {
            out = a;
            haveOut = true;
        } else {
            std::fprintf(stderr, "unexpected argument: %s\n", a.c_str());
            return 2;
        }
    }

    if (in.empty()) {
        usage(argv[0]);
        return 2;
    }
    if (!haveOut) out = binscp::scriptPathFor(in);

    std::vector<uint8_t> bytes;
    if (!readWholeFile(in, bytes)) {
        std::fprintf(stderr, "cannot read %s\n", in.c_str());
        return 1;
    }

    std::vector<binscp::BinRecord> records;
    std::string err;
    if (!binscp::parseBin(bytes.data(), bytes.size(), records, err)) {
        std::fprintf(stderr, "%s: %s\n", in.c_str(), err.c_str());
        return 1;
    }
    if (records.empty()) {
        std::fprintf(stderr, "%s: no records -- not a .bin capture?\n", in.c_str());
        return 1;
    }

    binscp::Report rep;
    const std::string script = binscp::toScript(records, in, opt, rep);

    if (!writeWholeFile(out, script)) {
        std::fprintf(stderr, "cannot write %s\n", out.c_str());
        return 1;
    }

    std::fputs(binscp::formatReport(rep, in).c_str(), stdout);
    std::printf("  wrote %s\n", out.c_str());
    return 0;
}
