/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2017 Lucas Thompson.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _TEST_HELPER_H_
#define _TEST_HELPER_H_

#include <initializer_list>
#include <memory>
#include <iostream>
#include <functional>

#include <QtTest>

namespace Test
{

template <class T>
using Factory = std::function<std::unique_ptr<T>()>;

template <class T, typename... Args>
auto createFactory(Args... FArgs) -> Factory<T>
{
  return [&]() { return std::unique_ptr<T> { new T {FArgs...} }; };
}

using TestStatus = int;

auto startTestRunner(
  std::initializer_list<Factory<QObject>> tests,
  int argc,
  char *argv[],
  QString testName,
  QString orgName = "sonic-visualiser"
) -> TestStatus
{
    int good = 0, bad = 0;

    QCoreApplication app(argc, argv);
    app.setOrganizationName(orgName);
    app.setApplicationName(testName);
    auto executeTest = [&](std::unique_ptr<QObject> t) {
        if (QTest::qExec(t.get(), argc, argv) == 0) ++good;
        else ++bad;
    };

    for (const auto& test : tests) {
        executeTest(test());
    }

    if (bad > 0) {
	    cerr << "\n********* " << bad << " test suite(s) failed!\n" << endl;
	    return 1;
    } else {
	    cerr << "All tests passed" << endl;
	    return 0;
    }
}

} // namespace

#endif