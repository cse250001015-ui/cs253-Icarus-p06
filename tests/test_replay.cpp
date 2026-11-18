// test/test_replay.cpp
// Unit tests for the file parsing in app/replay.cpp (ReplayReader). They check
// the formats of INTERFACE.md section 7 only; they do not check union-find
// behaviour (the counter checks against the generators belong to the Structure B tests).
// Built without -DNDEBUG so assert is active. Tests may use std containers.
#include "../app/replay.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using perc::ReplayCommand;
using perc::ReplayKind;
using perc::ReplayReader;

static const char* kTmp = "replay_test_tmp.txt";

static void writeFile(const std::string& text) {
    std::ofstream f(kTmp, std::ios::binary);
    f << text;
}

static std::vector<ReplayCommand> readAll(ReplayReader& rd) {
    std::vector<ReplayCommand> v;
    ReplayCommand c;
    while (rd.next(c)) v.push_back(c);
    return v;
}

static bool mentions(const std::string& msg, const char* part) {
    return msg.find(part) != std::string::npos;
}

static void testOrderFile() {
    writeFile("# comment\n3\n0\n4 # inline\n8\n");
    ReplayReader rd(kTmp);
    assert(rd.ok() && rd.kind() == ReplayKind::Order && rd.header() == 3);
    auto v = readAll(rd);
    assert(rd.ok() && v.size() == 3);
    assert(v[0].op == 'o' && v[0].a == 0);
    assert(v[1].a == 4 && v[1].line == 4);
    assert(v[2].a == 8);
}

static void testUnionsFile() {
    writeFile("# c\n\n5\nu 0 1   # unite\r\nf 2\n\nu 3 4\n");
    ReplayReader rd(kTmp);
    assert(rd.ok() && rd.kind() == ReplayKind::Unions && rd.header() == 5);
    auto v = readAll(rd);
    assert(rd.ok() && v.size() == 3);
    assert(v[0].op == 'u' && v[0].a == 0 && v[0].b == 1);
    assert(v[1].op == 'f' && v[1].a == 2);
    assert(v[2].op == 'u' && v[2].a == 3 && v[2].b == 4);
}

static void testEmptyFileKindFromExtension() {
    writeFile("4\n");
    ReplayReader rd(kTmp);                      // .txt, no records: defaults to Order
    assert(rd.ok() && rd.kind() == ReplayKind::Order);
    assert(readAll(rd).empty() && rd.ok());
}

static void testErrors() {
    {   // element out of range, error names the line
        writeFile("3\nu 0 1\nu 0 5\n");
        ReplayReader rd(kTmp);
        readAll(rd);
        assert(!rd.ok() && mentions(rd.error(), ":3:") && mentions(rd.error(), "range"));
    }
    {   // site index out of range for L*L
        writeFile("2\n0\n4\n");
        ReplayReader rd(kTmp);
        readAll(rd);
        assert(!rd.ok() && mentions(rd.error(), ":3:"));
    }
    {   // mixed kinds in one file
        writeFile("3\n0\nu 0 1\n");
        ReplayReader rd(kTmp);
        readAll(rd);
        assert(!rd.ok());
    }
    {   // wrong operand counts
        writeFile("3\nu 0\n");
        ReplayReader rd(kTmp);
        readAll(rd);
        assert(!rd.ok());
        writeFile("3\nf 0 1\n");
        ReplayReader rd2(kTmp);
        readAll(rd2);
        assert(!rd2.ok());
    }
    {   // unknown command, garbage number, trailing text
        writeFile("3\nx 0 1\n");
        ReplayReader a(kTmp);
        readAll(a);
        assert(!a.ok());
        writeFile("3\n1x\n");
        ReplayReader b(kTmp);
        readAll(b);
        assert(!b.ok());
    }
    {   // bad headers
        writeFile("");
        assert(!ReplayReader(kTmp).ok());
        writeFile("# only comments\n");
        assert(!ReplayReader(kTmp).ok());
        writeFile("0\n");
        assert(!ReplayReader(kTmp).ok());
        writeFile("abc\n");
        assert(!ReplayReader(kTmp).ok());
        writeFile("3 4\n");
        assert(!ReplayReader(kTmp).ok());
    }
    {   // L*L overflows int for an order file
        writeFile("50000\n0\n");
        assert(!ReplayReader(kTmp).ok());
    }
    {   // missing file
        assert(!ReplayReader("no_such_replay_file.txt").ok());
    }
}

int main() {
    testOrderFile();
    testUnionsFile();
    testEmptyFileKindFromExtension();
    testErrors();
    std::remove(kTmp);
    std::cout << "test_replay: all tests passed\n";
    return 0;
}
