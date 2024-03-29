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

#ifndef TEST_PITCH_H
#define TEST_PITCH_H

#include "../Pitch.h"
#include "../Preferences.h"

#include <QObject>
#include <QtTest>
#include <QDir>

#include <iostream>

using namespace std;
using namespace sv;

class TestPitch : public QObject
{
    Q_OBJECT

private slots:
    void init() {
        Preferences::getInstance()->setOctaveOfMiddleC(4);
        Preferences::getInstance()->setTuningFrequency(440);
    }

    void pitchLabel()
    {
        QCOMPARE(Pitch::getPitchLabel(60, 0, false), QString("C4"));
        QCOMPARE(Pitch::getPitchLabel(69, 0, false), QString("A4"));
        QCOMPARE(Pitch::getPitchLabel(61, 0, false), QString("C#4"));
        QCOMPARE(Pitch::getPitchLabel(61, 0, true), QString("Db4"));
        QCOMPARE(Pitch::getPitchLabel(59, 0, false), QString("B3"));
        QCOMPARE(Pitch::getPitchLabel(59, 0, true), QString("B3"));
        QCOMPARE(Pitch::getPitchLabel(0, 0, false), QString("C-1"));

        QCOMPARE(Pitch::getPitchLabel(60, -40, false), QString("C4-40c"));
        QCOMPARE(Pitch::getPitchLabel(60, 40, false), QString("C4+40c"));
        QCOMPARE(Pitch::getPitchLabel(58, 4, false), QString("A#3+4c"));

        Preferences::getInstance()->setOctaveOfMiddleC(3);

        QCOMPARE(Pitch::getPitchLabel(60, 0, false), QString("C3"));
        QCOMPARE(Pitch::getPitchLabel(69, 0, false), QString("A3"));
        QCOMPARE(Pitch::getPitchLabel(61, 0, false), QString("C#3"));
        QCOMPARE(Pitch::getPitchLabel(61, 0, true), QString("Db3"));
        QCOMPARE(Pitch::getPitchLabel(59, 0, false), QString("B2"));
        QCOMPARE(Pitch::getPitchLabel(59, 0, true), QString("B2"));
        QCOMPARE(Pitch::getPitchLabel(0, 0, false), QString("C-2"));

        QCOMPARE(Pitch::getPitchLabel(60, -40, false), QString("C3-40c"));
        QCOMPARE(Pitch::getPitchLabel(60, 40, false), QString("C3+40c"));
        QCOMPARE(Pitch::getPitchLabel(58, 4, false), QString("A#2+4c"));
    }

    void pitchLabelForFrequency()
    {
        QCOMPARE(Pitch::getPitchLabelForFrequency(440, 440, false), QString("A4"));
        QCOMPARE(Pitch::getPitchLabelForFrequency(440, 220, false), QString("A5"));
        QCOMPARE(Pitch::getPitchLabelForFrequency(261.63, 440, false), QString("C4"));
    }

#define MIDDLE_C 261.6255653005986

    void frequencyForPitch()
    {
        QCOMPARE(Pitch::getFrequencyForPitch(60, 0), MIDDLE_C);
        QCOMPARE(Pitch::getFrequencyForPitch(69, 0), 440.0);
        QCOMPARE(Pitch::getFrequencyForPitch(60, 0, 220), MIDDLE_C / 2.0);
        QCOMPARE(Pitch::getFrequencyForPitch(69, 0, 220), 220.0);
    }

    void pitchForFrequency()
    {
        double centsOffset = 0.0;
        QCOMPARE(Pitch::getPitchForFrequency(MIDDLE_C, &centsOffset), 60);
        QCOMPARE(centsOffset + 1.0, 1.0); // avoid ineffective fuzzy-compare to 0
        QCOMPARE(Pitch::getPitchForFrequency(261.0, &centsOffset), 60);
        QCOMPARE(int(centsOffset), -4);
        QCOMPARE(Pitch::getPitchForFrequency(440.0, &centsOffset), 69);
        QCOMPARE(centsOffset + 1.0, 1.0);
    }

    void pitchForFrequencyF()
    {
        float centsOffset = 0.f;
        QCOMPARE(Pitch::getPitchForFrequency(MIDDLE_C, &centsOffset), 60);
        QCOMPARE(centsOffset + 1.f, 1.f); // avoid ineffective fuzzy-compare to 0
        QCOMPARE(Pitch::getPitchForFrequency(261.0, &centsOffset), 60);
        QCOMPARE(int(centsOffset), -4);
        QCOMPARE(Pitch::getPitchForFrequency(440.0, &centsOffset), 69);
        QCOMPARE(centsOffset + 1.f, 1.f);
    }

    void melForFrequency()
    {
        auto check = [&](double freq, Pitch::MelFormula formula, double expected) {
            double actual = Pitch::getMelForFrequency(freq, formula);
            if (fabs(actual - expected) > 0.01) {
                cerr << "(failure with formula = " << int(formula) << ")" << endl;
                QCOMPARE(actual, expected);
            }
        };
        
        auto formula = Pitch::MelFormula::OShaughnessy;
        check(1000.0, formula, 999.9855);
        check(MIDDLE_C, formula, 357.8712);
        check(4000.0, formula, 2146.0645);

        formula = Pitch::MelFormula::Fant;
        check(1000.0, formula, 1000.0000);
        check(MIDDLE_C, formula, 335.2838);
        check(4000.0, formula, 2321.9281);

        formula = Pitch::MelFormula::Slaney;
        check(1000.0, formula, 15.0000);
        check(MIDDLE_C, formula, 3.9244);
        check(4000.0, formula, 35.1638);
    }
    
    void frequencyForMel()
    {
        // opposite arg order from above, so we can reuse the same calling code
        auto check = [&](double expected, Pitch::MelFormula formula, double mel) {
            double actual = Pitch::getFrequencyForMel(mel, formula);
            if (fabs(actual - expected) > 0.01) {
                cerr << "(failure with formula = " << int(formula) << ")" << endl;
                QCOMPARE(actual, expected);
            }
        };
        
        auto formula = Pitch::MelFormula::OShaughnessy;
        check(1000.0, formula, 999.9855);
        check(MIDDLE_C, formula, 357.8712);
        check(4000.0, formula, 2146.0645);

        formula = Pitch::MelFormula::Fant;
        check(1000.0, formula, 1000.0000);
        check(MIDDLE_C, formula, 335.2838);
        check(4000.0, formula, 2321.9281);

        formula = Pitch::MelFormula::Slaney;
        check(1000.0, formula, 15.0000);
        check(MIDDLE_C, formula, 3.9244);
        check(4000.0, formula, 35.16376);
    }
    
    void melForFrequencyAndBack()
    {
        for (int form = 0; form < 3; ++form) {
            Pitch::MelFormula formula = (Pitch::MelFormula)form;
            for (int i = 0; i < 40; ++i) {
                double freq = i * 200.0;
                double mel = Pitch::getFrequencyForMel(freq, formula);
                double back = Pitch::getMelForFrequency(mel, formula);
                QCOMPARE(back, freq);
            }
        }
    }
};

#endif
