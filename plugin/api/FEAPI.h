////////////////////////////////////////////////////////////////////////////////////
//     /*!  \file FEAPI.h 
//          \brief interface of the feature extraction plugin. 
//        <br><br>
//        In the following, context function pointers typedefs are referred to as 
//        functions.<br>
//
//        
//        Create a new instance of the plugin with the function 
//        ::FEAPI_CreatePluginInstance. The call of this function is mandatory.
//        <br><br>
//
//        Initialize the plugin with the call of function
//        ::FEAPI_InitializePlugin.<br><br>
//
//        Information about the plugin can be requested via the following 
//        functions:<br>
//        ::FEAPI_GetPluginAPIVersion, <br>
//        ::FEAPI_GetPluginName, <br> 
//        ::FEAPI_GetPluginVendor, <br>
//        ::FEAPI_GetPluginVendorVersion, <br>
//        ::FEAPI_GetPluginCopyright, <br>
//        ::FEAPI_GetPluginDescription. <br>
//        The call of these functions is optional. Combined with a call to
//        ::FEAPI_GetPluginId, the plugin can be uniquely identified. <br><br>
//
//        The technical capabilities of the plugin can be requested via the call 
//        of the function
//        ::FEAPI_GetPluginProperty.<br><br>
//
//        To get the number of the features resp. results that are computed 
//        by the plugin, call the function <br>
//        ::FEAPI_GetPluginNumOfResults; <br>
//        the function <br>
//        ::FEAPI_GetPluginResultDescription gives you detailed information about the 
//        meaning and usage of every result (see structure ::_ResultDescription
//        for details). <br><br>
//
//        To get the number of the options/ parameter settings that can be 
//        done before processing, call the function <br>
//        ::FEAPI_GetPluginNumOfParameters; <br>
//        the function <br>
//        ::FEAPI_GetPluginParamDescription gives you detailed information about 
//        the meaning and usage of every single parameter (see structure 
//        ::FEAPI_ParameterDescription_t for details).<br><br>
//
//        To get or set a specific parameter value, call the function <br>
//        ::FEAPI_GetPluginParameter resp. <br>
//        ::FEAPI_SetPluginParameter. <br><br>
//
//        After the plugin is initialized, the actual processing can begin.
//        The ::FEAPI_ProcessPlugin <br>
//        function can be called to do the actual feature/result 
//        calculation. ::FEAPI_ProcessPlugin expects subsequently new blocks 
//        of audio data. Note that ::FEAPI_ProcessPlugin does not return 
//        computed feature values.<br>
//
//        After finishing one ::FEAPI_ProcessPlugin call, zero, one or more 
//        results can be available, depending on the plug-ins implementation. 
//        To query the available number of values for every feature/result, 
//        call <br>
//        ::FEAPI_GetPluginSizeOfResult, <br>
//        which returns the number of values for this
//        result. Afterwards, the result values for each result can
//        be requested via <br>
//        ::FEAPI_GetPluginResult. Note that the memory for the results has to be 
//        allocated by the host.<br><br>
//
//        To signal that no more audio data is available at the end (e.g. of
//        the audio file), call <br>
//        ::FEAPI_ProcessPluginDone and get the last results with 
//        ::FEAPI_GetPluginResult if available.<br><br>
//
//        To flush the internal buffers, the function <br>
//        ::FEAPI_ResetPlugin <br>
//        may be called.
//
//        After all processing has been done, destroy the instance of the 
//        plugin with the function <br>
//        ::FEAPI_DestroyPluginInstance. <br>
//        The call of this function is mandatory. <br><br>
//
//        <br><br><br>
//        The Feature Extraction plugin API is released under a BSD style
//        license. Please make all changes available to the authors.<br>
//        Contact information: lerch <at> zplane.de.
//
//     */
//
//        Copyright (c) 2004-2005, Alexander Lerch, zplane.development GbR
//        All rights reserved.
//
//        Redistribution and use in source and binary forms, with or without 
//        modification, are permitted provided that the following conditions 
//        are met:
//
//        *   Redistributions of source code must retain the above copyright 
//            notice, this list of conditions and the following disclaimer.
//        *   Redistributions in binary form must link to the FEAPI website
//            http://www.sf.net/projects/feapi,
//            reproduce this list of conditions and the following 
//            disclaimer in the documentation and/or other materials 
//            provided with the distribution.
//        *   The name of the contributors to this software must not be used 
//            to endorse or promote products derived from this software 
//            without specific prior written permission.
//
//        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//        FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
//        COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
//        INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//        BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
//        LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
//        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
//        LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
//        ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
//        POSSIBILITY OF SUCH DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////////
//  CVS INFORMATION
//
//  $RCSfile: FEAPI.h,v $
//  $Author: alex_lerch $
//  $Date: 2005/05/20 17:08:36 $
//
//  $Log: FEAPI.h,v $
//  Revision 1.2  2005/05/20 17:08:36  alex_lerch
//  - updated documentation
//  - added "signal" typedef for inputs and results
//  - changed function PluginCanDo to PluginGetProperty and changed the function itself to return values instead of bools
//  - cosmetic changes
//
//  Revision 1.1.1.1  2005/03/30 14:54:40  alex_lerch
//  first draft version requiring several updates:
//  - interface check
//  - check of plugin base class
//  - implementation of host base class
//  - update of outdated documentation
//
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//                      !!!Do never ever edit this file!!!
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/*! \brief avoid multiple header includes */
#if !defined(FEAPI_HEADER_INCLUDED) 
#define FEAPI_HEADER_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

/* Maximum string lengths. */
const unsigned int FEAPI_uiMaxNameLength        = 1024; //!< maximum number of characters for a name string (including null terminator)
const unsigned int FEAPI_uiMaxUnitLength        = 1024; //!< maximum number of characters for a unit string (including null terminator)
const unsigned int FEAPI_uiMaxDescriptionLength = 4096; //!< maximum number of characters for a description string (including null terminator)

////////////////////////////////////////////////////////////////////////////////////
// interface structures and types
/** @defgroup types Interface Structures and Types
 *  @{
 */


/*! Structure describing properties and other information about one result/feature. */
typedef struct FEAPI_SignalDescription_t_tag
{
    char    acName[FEAPI_uiMaxNameLength];                //!< name of the result/feature (e.g. "Spectral Centroid", "Loudness", etc.) 
    char    acUnit[FEAPI_uiMaxUnitLength];                //!< unit of the result/feature (e.g. "dB", "sone", "Hz", etc.) 
    char    acDescription[FEAPI_uiMaxDescriptionLength];  //!< description of the result/feature (clear text description) 
    float   fRangeMin;              //!< minimum value of the result/feature (if no minimum value: minimum floating point value) 
    float   fRangeMax;              //!< maximum value of the result/feature (if no maximum value: maximum floating point value) 
    float   fQuantizedTo;           //!< quantization step size of the result/feature (e.g. 1 for integer result, -1 for no quantization) 
    float   fSampleRate;            //!< sample rate of the result/feature output in Hz; -1 if sample rate equals input block length, -2 for non-equidistant samples
} FEAPI_SignalDescription_t;


/*! Structure describing properties and other information about one parameter. */
typedef struct FEAPI_ParameterDescription_t_tag
{
    char    acName[FEAPI_uiMaxNameLength];                //!< name of the parameter (e.g. "Gain", "Sensitivity", etc.) 
    char    acUnit[FEAPI_uiMaxUnitLength];                //!< unit of the parameter (e.g. "dB", "Hz", etc.) 
    char    acDescription[FEAPI_uiMaxDescriptionLength];  //!< description of the parameter (clear text description) 
    float   fRangeMin,              //!< minimum value of the parameter (if no minimum value: minimum floating point value) 
            fRangeMax,              //!< maximum value of the parameter (if no maximum value: maximum floating point value) 
            fDefaultValue;          //!< default value for the parameter 
    float   fQuantizedTo;           //!< quantization step size of the parameter (e.g. 1 for integer quantization, -1 for no quantization) 
    int     bIsChangeableInRealTime;//!< 0/false if the parameter has to be set before the processing starts and can not be changed during processing, 1 if the parameter can be changed during processing
} FEAPI_ParameterDescription_t;


/*! Structure for user/vendor defined commands. */
typedef struct FEAPI_UserData_t_tag
{
    char    *pcUserString;          //!< user defined string value (memory could be freed by host after function call)
    void    *pcUserData;            //!< pointer to user defined data
} FEAPI_UserData_t;


/*! Typedef for the plugin instance handle. */
typedef void* FEAPI_PluginInstance_t;


/*! This typedef is used for time stamps in seconds. 
    For a block of data, the time stamp is defined to be the time at the beginning of the block. */
typedef double FEAPI_TimeStamp_t;


/*! This typedef is used for input and output data. 
    At least for this version of the API, this is exactly float. */
typedef float FEAPI_Signal_t;


/*! Enumerator for possible error return values, if any other value than FEAPI_kNoError is returned, 
    the function was not able to execute the specified operation(s).<br>
    All error defines are negative. */
typedef enum FEAPI_Error_t_tag
{
    FEAPI_kNoError           = 0,       //!< no error occurred
    FEAPI_kUnspecifiedError  = -1,      //!< an error occurred, but the type is not yet specified
    FEAPI_kUnknownError      = -9999    //!< an error occurred, but its type is not specifyable
} FEAPI_Error_t;


/*! Enumerator for retrieval of version info which can be resolved as major.minor.sub. */
typedef enum FEAPI_VersionInfo_t_tag
{
    FEAPI_kMajorVersion    = 0,        //!< indicates the major version
    FEAPI_kMinorVersion    = 1,        //!< indicates the minor version
    FEAPI_kSubVersion      = 2         //!< indicates the sub version or bug-fix version
} FEAPI_VersionInfo_t;


/*! Enumerator for retrieval of information about what the plug supports. */
typedef enum FEAPI_PluginProperty_t_tag
{
    FEAPI_kMinSampleRate    = 0,        //!< indicates the minimum sample rate
    FEAPI_kMaxSampleRate    = 1,        //!< indicates the maximum sample rate
    FEAPI_kMinChannels      = 2,        //!< indicates minimum number of channels
    FEAPI_kMaxChannels      = 3,        //!< indicates maximum number of channels
    FEAPI_kMinFrameSize     = 4,        //!< indicates minimum number of frames per process call
    FEAPI_kMaxFrameSize     = 5,        //!< indicates maximum number of frames per process call
    FEAPI_kOptFrameSize     = 6         //!< indicates optimal number of frames per process call
} FEAPI_PluginProperty_t;

/** @} */ 

////////////////////////////////////////////////////////////////////////////////////
// API function declaration
/** @defgroup apifun API function pointers
 *  @{
 */

    /*!
     * Creates a new instance of the plugin
     *
     * @param phInstanceHandle : handle to the instance to be created
     * @return FEAPI_Error_t  : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t   (*FEAPI_CreatePluginInstance_t) ( FEAPI_PluginInstance_t *phInstanceHandle );

    /*!
     * Destroys an instance of the plugin.
     *
     * @param phInstanceHandle : handle to the instance to be destroyed
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t   (*FEAPI_DestroyPluginInstance_t) ( FEAPI_PluginInstance_t *phInstanceHandle );

    /*!
     * initializes a new instance of the plugin
     *
     * @param hInstanceHandle : handle to the instance
     * @param fInputSampleRate : sample rate of input(s) in Hz
     * @param iNumberOfAudioChannels : number of input audio channels
     * @param iHostApiMajorVersion : major version number of host
     * @param pstUserData : pointer to user or vendor defined data (may be NULL)
     *
     * @return FEAPI_Error_t  : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t   (*FEAPI_InitializePlugin_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                            float               fInputSampleRate, 
                                                            int                 iNumberOfAudioChannels,
                                                            int                 iHostApiMajorVersion,
                                                            FEAPI_UserData_t     *pstUserData);


    /*! 
     * Gets the version number (major, minor or subversion) of the API used by the plugin.
     * This is *not* the plugin version, therefore the function may be called without a previously 
     * created instance.
     * 
     * @param eAPIMajorMinorOrSubVersion : flag which version type is requested
     * @return int  : requested version number 
     */
    typedef int (*FEAPI_GetPluginAPIVersion_t) ( FEAPI_VersionInfo_t eAPIMajorMinorOrSubVersion ); //!< \todo change ints to (unsigned) ints where appropriate?


    /*!
     * Gets the name of the plugin.
     *
     * @param hInstanceHandle : handle to instance 
     * @param *pcPluginName : pointer to buffer of FEAPI_uiMaxNameLength chars, the name will be copied to this buffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginName_t) ( FEAPI_PluginInstance_t hInstanceHandle, 
                                                                    char    *pcPluginName);

    /*!
     * Gets the vendor name of the plugin.
     *
     * @param hInstanceHandle : handle to instance 
     * @param *pcPluginVendor : pointer to buffer of FEAPI_uiMaxNameLength chars, the vendor name will be copied to this buffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginVendor_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                    char    *pcPluginVendor);

    /*!
     * Gets an indication of the plugins capabilities.
     *
     * @param hInstanceHandle : handle to instance 
     * @param ePluginProperty : requested property 
     * @return float  : corresponding value
     */
    typedef float (*FEAPI_GetPluginProperty_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                FEAPI_PluginProperty_t ePluginProperty);

    /*!
     * Gets the (vendor unique) plugin identification string.
     *
     * @param hInstanceHandle : handle to instance 
     * @param *pcPluginId : pointer to buffer of FEAPI_uiMaxNameLength chars, the id will be copied to this buffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginId_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                    char *pcPluginId);

    /*!
     * Gets the vendor version of the plugin.
     *
     * @param hInstanceHandle : handle to instance 
     * @param ePluginMajorMinorOrSubVersion : flag which version type is requested
     * @return int  : requested version number 
     */
    typedef int (*FEAPI_GetPluginVendorVersion_t) ( FEAPI_PluginInstance_t  hInstanceHandle, 
                                                    FEAPI_VersionInfo_t     ePluginMajorMinorOrSubVersion); 

    /*!
     * Gets the description of the plugin, containing information about what the plugin actually does.
     *
     * @param hInstanceHandle : handle to instance 
     * @param *pcPluginDescription : pointer to buffer of FEAPI_uiMaxDescriptionLength chars, the plugin description will be copied to this buffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginDescription_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                            char *pcPluginDescription);

    /*!
     * Gets the copyright information for the plugin.
     *
     * @param hInstanceHandle : handle to instance 
     * @param *pcPluginCopyright : pointer to buffer of FEAPI_uiMaxDescriptionLength chars, the plugin copyright information will be copied to this buffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginCopyright_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                          char *pcPluginCopyright);

    /*!
     * Gets the number of inputs for the plugin. This number will equal the 
     * number of audio channels in many cases, otherwise exceed the number of audio channels. 
     * The additional input channels are plugin developer specific and not recommended. If used,
     * they have to be routed host internally.
     *
     * @param hInstanceHandle : handle to instance 
     * @return int  : number of inputs
     */
    typedef int (*FEAPI_GetPluginNumOfInputs_t) (FEAPI_PluginInstance_t hInstanceHandle);

    /*!
     * Gets information about one of the possible inputs.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iInputIndex : index of the input the description is requested for, index ranges from 0...NumOfInputs-1
     * @param *pstInputDescription : the requested description will be copied to this structure
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginInputDescription_t) (FEAPI_PluginInstance_t      hInstanceHandle, 
                                                                int                         iInputIndex, 
                                                                FEAPI_SignalDescription_t   *pstInputDescription);

    /*!
     * Gets the number of parameters.
     *
     * @param hInstanceHandle : handle to instance 
     * @return int  : number of parameters
     */
    typedef int (*FEAPI_GetPluginNumOfParameters_t) (FEAPI_PluginInstance_t hInstanceHandle);

    /*!
     * Gets information about one of the possible parameters.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iParameterIndex : index of the parameter (setting) the description is requested for, index ranges from 0...NumOfParameters-1
     * @param *pstParameterDescription : the requested description will be copied to this structure
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginParameterDescription_t) (FEAPI_PluginInstance_t          hInstanceHandle, 
                                                                    int                             iParameterIndex, 
                                                                    FEAPI_ParameterDescription_t    *pstParameterDescription);

    /*!
     * Sets a parameter to a specified value.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iParameterIndex : index of the parameter (setting) to be changed, index ranges from 0...NumOfParameters-1
     * @param fValue : new value of the parameter
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_SetPluginParameter_t) (   FEAPI_PluginInstance_t  hInstanceHandle, 
                                                            int                     iParameterIndex, 
                                                            float                   fValue);

    /*!
     * Gets the current value of a parameter.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iParameterIndex : index of the parameter (setting) requested, index ranges from 0...NumOfParameters-1
     * @return float  : value of the parameter with index iParameterIndex
     */
    typedef float (*FEAPI_GetPluginParameter_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                    int iParameterIndex);

    /*!
     * Processes a block of audio data.
     *
     * @param hInstanceHandle : handle to instance 
     * @param **ppfInputBuffer : input audio data in the format [channels][samples]; audio samples have a range from -1.0...+1.0
     * @param *ptFEAPI_TimeStamp : time stamps in seconds for every input, may be NULL
     * @param iNumberOfFrames : number of frames in ppfInputBuffer
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_ProcessPlugin_t) (FEAPI_PluginInstance_t  hInstanceHandle, 
                                                    const FEAPI_Signal_t    **ppfInputBuffer, 
                                                    const FEAPI_TimeStamp_t *ptFEAPI_TimeStamp, 
                                                    int                     iNumberOfFrames);           

    /*!
     * Signals that no more input data is available (does the final processing).
     *
     * @param hInstanceHandle : handle to instance 
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_ProcessPluginDone_t) (FEAPI_PluginInstance_t hInstanceHandle);

    /*!
     * Gets the number of results/features to be calculated.
     *
     * @param hInstanceHandle : handle to instance 
     * @return int  : number of results
     */
    typedef int (*FEAPI_GetPluginNumOfResults_t) (FEAPI_PluginInstance_t hInstanceHandle);

    /*!
     * Gets information about one of the possible results/features.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iResultIndex : index of the result (feature) the description is requested for, index ranges from 0...NumOfResults-1
     * @param *pstResultDescription : the requested description will be copied to this structure
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginResultDescription_t) (   FEAPI_PluginInstance_t      hInstanceHandle, 
                                                                    int                         iResultIndex, 
                                                                    FEAPI_SignalDescription_t   *pstResultDescription);

    /*!
     * Gets the size of one result in FEAPI_Signal_t values (4 bytes per value).
     *
     * @param hInstanceHandle : handle to instance 
     * @param iResultIndex : index of the result/feature the size is requested for, index ranges from 0...NumOfResults-1
     * @return int : size of result in FEAPI_Signal_t values (4 bytes per value)
     */
    typedef int (*FEAPI_GetPluginSizeOfResult_t) (FEAPI_PluginInstance_t hInstanceHandle, 
                                                                    int iResultIndex);

    /*!
     * Gets one result.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iResultIndex : index of the requested result/feature, index ranges from 0...NumOfResults-1
     * @param *pfResult : the result data is copied into this buffer
     * @param *ptFEAPI_TimeStamp : the time stamp of the result
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_GetPluginResult_t) (  FEAPI_PluginInstance_t  hInstanceHandle, 
                                                        int                     iResultIndex, 
                                                        FEAPI_Signal_t          *pfResult, 
                                                        FEAPI_TimeStamp_t       *ptFEAPI_TimeStamp);           

    /*!
     * Gets the maximum latency of one result. Since the host buffer size may vary, this is only the 
     * *internal* latency.
     *
     * @param hInstanceHandle : handle to instance 
     * @param iResultIndex : index of the requested result (feature), index ranges from 0...NumOfResults-1
     * @return int  : number of samples (at audio input sample rate) which is required to calculate this result the first time (negative values are not allowed, 0 means undefined)
     */
    typedef int (*FEAPI_GetPluginResultLatency_t) (FEAPI_PluginInstance_t hInstanceHandle, int iResultIndex); 

    /*!
     * Resets/clears all internal buffers and states, so that the plugin is in a state where it can start processing a new audio stream.
     *
     * @param hInstanceHandle : handle to instance 
     * @return FEAPI_Error_t : FEAPI_kNoError when no error
     */
    typedef FEAPI_Error_t (*FEAPI_ResetPlugin_t) (FEAPI_PluginInstance_t hInstanceHandle);

/** @} */ 


#ifdef __cplusplus
}
#endif


#endif // #if !defined(FEAPI_HEADER_INCLUDED)
