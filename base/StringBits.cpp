/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2010 Chris Cannam.
*/

#include "StringBits.h"

#include "Debug.h"

using namespace std;

double
StringBits::stringToDoubleLocaleFree(QString s, bool *ok)
{
    int dp = 0;
    int sign = 1;
    int i = 0;
    double result = 0.0;
    int len = s.length();

    result = 0.0;

    if (ok) *ok = true;

    while (i < len && s[i].isSpace()) ++i;
    if (i < len && s[i] == '-') sign = -1;

    while (i < len) {

        QChar c = s[i];

        if (c.isDigit()) {

            double d = c.digitValue();

            if (dp > 0) {
                for (int p = dp; p > 0; --p) d /= 10.0;
                ++dp;
            } else {
                result *= 10.0;
            }

            result += d;

        } else if (c == '.') {

            dp = 1;

        } else if (ok) {
            *ok = false;
        }

        ++i;
    }

    return result * sign;
}
    
QStringList
StringBits::splitQuoted(QString s, QChar separator, EscapeMode escapeMode)
{
    QStringList tokens;
    QString tok;

    // beg -> at beginning of line
    // sep -> just seen a field separator
    // unq -> in an unquoted field
    // q1  -> in a single-quoted field
    // q2  -> in a double-quoted field

    enum { beg, sep, unq, q1, q2 } mode = beg;

    bool use_doubling = (escapeMode == EscapeDoubling ||
                         escapeMode == EscapeAny);
    bool use_backslash = (escapeMode == EscapeBackslash ||
                          escapeMode == EscapeAny);

    for (int i = 0; i < s.length(); ++i) {
        
        QChar c = s[i];

        if (c == '\'') {
            switch (mode) {
            case beg: case sep: mode = q1; break;
            case unq: case q2: tok += c; break;
            case q1:
                if (use_doubling && i+1 < s.length() && s[i+1] == c) {
                    tok += c; ++i; break;
                } else {
                    mode = unq; break;
                }
            }

        } else if (c == '"') {
            switch (mode) {
            case beg: case sep: mode = q2; break;
            case unq: case q1: tok += c; break;
            case q2: 
                if (use_doubling && i+1 < s.length() && s[i+1] == c) {
                    tok += c; ++i; break;
                } else {
                    mode = unq; break;
                }
            }

        } else if (c == separator || (separator == ' ' && c.isSpace())) {
            switch (mode) {
            case beg: mode = sep; tokens << ""; break;
            case sep: if (separator != ' ') tokens << ""; break;
            case unq: mode = sep; tokens << tok; tok = ""; break;
            case q1: case q2: tok += c; break;
            }

        } else if (c == '\\' && use_backslash) {
            if (++i < s.length()) {
                c = s[i];
                switch (mode) {
                case beg: case sep: mode = unq; tok += c; break;
                case unq: case q1: case q2: tok += c; break;
                }
            }

        } else {
            switch (mode) {
            case beg: case sep: mode = unq; tok += c; break;
            case unq: case q1: case q2: tok += c; break;
            }
        }
    }

    if (tok != "" || mode != beg) {
        if (mode == q1) {
            tokens << ("'" + tok);  // turns out it wasn't quoted after all
        } else if (mode == q2) {
            tokens << ("\"" + tok);
        } else {
            tokens << tok;
        }
    }

    return tokens;
}

QStringList
StringBits::split(QString line, QChar separator, bool quoted)
{
    if (quoted) {
        return splitQuoted(line, separator);
    } else {
        return line.split(separator,
                          separator == ' ' ? QString::SkipEmptyParts :
                          QString::KeepEmptyParts);
    }
}

QString
StringBits::joinDelimited(QVector<QString> row, QString delimiter)
{
    QString s;
    for (auto col: row) {
        if (s != "") {
            s += delimiter;
        }
        if (col.contains(delimiter)) {
            col.replace("\"", "\"\"");
            col = "\"" + col + "\"";
        }
        s += col;
    }
    return s;    
}

bool
StringBits::isValidUtf8(const std::string &bytes, bool isTruncated)
{
    size_t len = bytes.length();
    size_t mblen = 0;
    unsigned char first = '\0';
    
    for (size_t i = 0; i < len; ++i) {

	unsigned char c = bytes[i];

	if (((c & 0xc0) == 0xc0) || !(c & 0x80)) {

	    // 11xxxxxx or 0xxxxxxx: first byte of a character sequence

	    if (mblen > 0) {
                
		// have we seen a valid sequence?
		size_t length = 
		    (!(first & 0x20)) ? 2 :
		    (!(first & 0x10)) ? 3 :
		    (!(first & 0x08)) ? 4 :
		    (!(first & 0x04)) ? 5 : 0;

                if (length != 0 && mblen != length) {
                    // previous multibyte sequence had invalid length
                    return false;
                }
            }

            mblen = 0;
            first = c;

        } else {
            
	    // second or subsequent byte

	    if (mblen == 0) {
                // ... without a first byte!                
                return false;
            }
        }
    }

    // at the end

    if (isTruncated) {
        // can't trust any errors arising now
        return true;
    }

    if (mblen > 0) {
                
        // have we seen a valid sequence?
        size_t length = 
            (!(first & 0x20)) ? 2 :
            (!(first & 0x10)) ? 3 :
            (!(first & 0x08)) ? 4 :
            (!(first & 0x04)) ? 5 : 0;
        
        if (length != 0 && mblen != length) {
            // final multibyte sequence had invalid length
            return false;
        }
    }

    return true;
}


        

