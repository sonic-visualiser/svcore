/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
   
    This is experimental software.  Not for distribution.
*/

#include "BeatDetectTransform.h"

#include "model/DenseTimeValueModel.h"
#include "model/SparseOneDimensionalModel.h"

#include <iostream>
#include "dsp/onsets/DetectionFunction.h"
#include "dsp/tempotracking/TempoTrack.h"


BeatDetectTransform::BeatDetectTransform(Model *inputModel) :
    Transform(inputModel)
{
    // Step resolution for the detection function in seconds
    double stepSecs = 0.01161;

    // Step resolution for the detection function in samples
    size_t stepSize = (size_t)floor((double)inputModel->getSampleRate() * 
				    stepSecs); 


//    m_w->m_bdf->setResolution(stepSize);
//    output->setResolution(stepSize);

    std::cerr << "BeatDetectTransform::BeatDetectTransform: input sample rate " << inputModel->getSampleRate() << ", stepSecs " << stepSecs << ", stepSize " << stepSize << ", unrounded stepSize " << double(inputModel->getSampleRate()) * stepSecs << ", output sample rate " << inputModel->getSampleRate() / stepSize << ", unrounded output sample rate " << double(inputModel->getSampleRate()) / double(stepSize) << std::endl;

    m_output = new SparseOneDimensionalModel(inputModel->getSampleRate(), 1);
}

BeatDetectTransform::~BeatDetectTransform()
{
    // parent does it all
}

TransformName
BeatDetectTransform::getName()
{
    return tr("Beats");
}

void
BeatDetectTransform::run()
{
    SparseOneDimensionalModel *output = getOutput();
    DenseTimeValueModel *input = getInput();
    if (!input) return;

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

//    m_w->m_bdf->setResolution(stepSize);
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

    std::cerr << "Detecting beats at step size " << stepSize << "..." << std::endl;

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

//	m_w->m_bdf->addPoint(SparseTimeValueModel::Point
//			     (i * stepSize, dfOutput[i],
//			      QString("%1").arg(dfOutput[i])));
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

//    m_w->m_bdf->setCompletion(100);

    // Tempo Track Object instantiation and configuration
    TempoTrack tempoTracker(ttparams);

    // Vector of detected onsets
    vector<int> beats; 

    std::cerr << "Running tempo tracker..." << std::endl;

    beats = tempoTracker.process(dfOutput, blocks);

    delete [] buffer;
    delete [] dfOutput;
    delete [] aCoeffs;
    delete [] bCoeffs;

    for (size_t i = 0; i < beats.size(); ++i) {
//	std::cerr << "Beat value " << beats[i] << ", multiplying out to " << beats[i] * stepSize << std::endl;
	float bpm = 0.0;
	int fdiff = 0;
	if (i < beats.size() - 1) {
	    fdiff = (beats[i+1] - beats[i]) * stepSize;
	    // one beat is fdiff frames, so there are samplerate/fdiff bps,
	    // so 60*samplerate/fdiff bpm
	    if (fdiff > 0) {
		bpm = (60.0 * input->getSampleRate()) / fdiff;
	    }
	}
	output->addPoint(SparseOneDimensionalModel::Point
			 (origin + beats[i] * stepSize, QString("%1").arg(bpm)));
	if (m_deleting) return;
    }

    output->setCompletion(100);
}

DenseTimeValueModel *
BeatDetectTransform::getInput()
{
    DenseTimeValueModel *dtvm =
	dynamic_cast<DenseTimeValueModel *>(getInputModel());
    if (!dtvm) {
	std::cerr << "BeatDetectTransform::getInput: WARNING: Input model is not conformable to DenseTimeValueModel" << std::endl;
    }
    return dtvm;
}

SparseOneDimensionalModel *
BeatDetectTransform::getOutput()
{
    return static_cast<SparseOneDimensionalModel *>(getOutputModel());
}

