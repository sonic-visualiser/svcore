TEMPLATE = lib

SV_UNIT_PACKAGES = vamp vamp-hostsdk
load(../sv.prf)

CONFIG += sv staticlib qt thread warn_on stl rtti exceptions
QT += xml

TARGET = svtransform

DEPENDPATH += . .. 
INCLUDEPATH += . ..
OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += FeatureExtractionModelTransformer.h \
           RealTimeEffectModelTransformer.h \
           Transform.h \
           TransformDescription.h \
           TransformFactory.h \
           ModelTransformer.h \
           ModelTransformerFactory.h
SOURCES += FeatureExtractionModelTransformer.cpp \
           RealTimeEffectModelTransformer.cpp \
           Transform.cpp \
           TransformFactory.cpp \
           ModelTransformer.cpp \
           ModelTransformerFactory.cpp
