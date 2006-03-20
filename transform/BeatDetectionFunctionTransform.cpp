/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "BeatDetectionFunctionTransform.h"

#include "model/DenseTimeValueModel.h"
#include "model/SparseTimeValueModel.h"

#include <iostream>
#include "dsp/onsets/DetectionFunction.h"
#include "dsp/tempotracking/TempoTrack.h"


BeatDetectionFunctionTransform::BeatDetectionFunctionTransform(Model *inputModel) :
    Transform(inputModel)
{
    m_output = new SparseTimeValueModel(inputModel->getSampleRate(), 1,
					0.0, 0.0, false);
}

BeatDetectionFunctionTransform::~BeatDetectionFunctionTransform()
{
    // parent does it all
}

TransformName
BeatDetectionFunctionTransform::getName()
{
    return tr("Beat Detection Function");
}

void
BeatDetectionFunctionTransform::run()
{
    SparseTimeValueModel *output = getOutput();
    DenseTimeValueModel *input = getInput();
    if (!input) {
	std::cerr << "BeatDetectionFunctionTransform::run: WARNING: Input model is not conformable to DenseTimeValueModel" << std::endl;
	return;
    }

    DFConfig config;

    config.DFType = DF_COMPLEXSD;

    // Step resolution for the detection function in seconds
    config.stepSecs = 0.01161;

    // Step resolution for the detection function in samples
    config.stepSize = (unsigned int)floor((double)input->getSampleRate() * 
					  config.stepSecs ); 

    config.frameLength = 2 * config.stepSize;

    unsigned int stepSize = config.stepSize;
    unsigned int frameLength = config.frameLength;

    output->setResolution(stepSize);

    //Tempo Tracking Configuration Parameters
    TTParams ttparams;
    
    // Low Pass filter coefficients for detection function smoothing
    double* aCoeffs = new double[3];
    double* bCoeffs = new double[3];
	
    aCoeffs[ 0 ] = 1;
    aCoeffs[ 1 ] = -0.5949;
    aCoeffs[ 2 ] = 0.2348;
    bCoeffs[ 0 ] = 0.1600;
    bCoeffs[ 1 ] = 0.3200;
    bCoeffs[ 2 ] = 0.1600;

    ttparams.winLength = 512;
    ttparams.lagLength = 128;
    ttparams.LPOrd = 2;
    ttparams.LPACoeffs = aCoeffs;
    ttparams.LPBCoeffs = bCoeffs; 
    ttparams.alpha = 9;
    ttparams.WinT.post = 8;
    ttparams.WinT.pre = 7;

    ////////////////////////////////////////////////////////////
    // DetectionFunction
    ////////////////////////////////////////////////////////////
    // Instantiate and configure detection function object

    DetectionFunction df(config);

    size_t origin = input->getStartFrame();
    size_t frameCount = input->getEndFrame() - origin;
    size_t blocks = (frameCount / stepSize);
    if (blocks * stepSize < frameCount) ++blocks;

    double *buffer = new double[frameLength];

    // DF output with causal extension
    unsigned int clen = blocks + ttparams.winLength;
    double *dfOutput = new double[clen];

    std::cerr << "Running beat detection function at step size " << stepSize << "..." << std::endl;

    for (size_t i = 0; i < clen; ++i) {

//	std::cerr << "block " << i << "/" << clen << std::endl;
//	std::cerr << ".";

	if (i < blocks) {
	    size_t got = input->getValues(-1, //!!! needs to come from parent layer -- which is not supposed to be in scope at this point
					  origin + i * stepSize,
					  origin + i * stepSize + frameLength,
					  buffer);
	    while (got < frameLength) buffer[got++] = 0.0;
	    dfOutput[i] = df.process(buffer);
	} else {
	    dfOutput[i] = 0.0;
	}

	output->addPoint(SparseTimeValueModel::Point
			 (i * stepSize, dfOutput[i],
			  QString("%1").arg(dfOutput[i])));
//	m_w->m_bdf->setCompletion(i * 99 / clen);
	output->setCompletion(i * 99 / clen);

	if (m_deleting) {
	    delete [] buffer;
	    delete [] dfOutput;
	    delete [] aCoeffs;
	    delete [] bCoeffs;
	    return;
	}
    }

    output->setCompletion(100);
}

DenseTimeValueModel *
BeatDetectionFunctionTransform::getInput()
{
    return dynamic_cast<DenseTimeValueModel *>(getInputModel());
}

SparseTimeValueModel *
BeatDetectionFunctionTransform::getOutput()
{
    return static_cast<SparseTimeValueModel *>(getOutputModel());
}

