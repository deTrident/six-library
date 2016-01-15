/* =========================================================================
 * This file is part of six.sidd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2015, MDA Information Systems LLC
 *
 * six.sidd-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <sstream>

#include <six/SICommonXMLParser10x.h>
#include <six/sidd/DerivedDataBuilder.h>
#include <six/sidd/DerivedXMLParser110.h>

namespace
{
typedef xml::lite::Element* XMLElem;
typedef xml::lite::Attributes XMLAttributes;

template <typename T>
bool isDefined(const T& enumVal)
{
    return six::Init::isDefined(enumVal.value);
}

template <typename T>
bool isUndefined(const T& enumVal)
{
    return six::Init::isUndefined(enumVal.value);
}

template <typename SmartPtrT>
void confirmNonNull(const SmartPtrT& ptr,
                    const std::string& name,
                    const std::string& suffix = "")
{
    if (ptr.get() == NULL)
    {
        std::string msg = name + " is required";
        if (!suffix.empty())
        {
            msg += " " + suffix;
        }

        throw except::Exception(Ctxt(msg));
    }
}
}

namespace six
{
namespace sidd
{
const char DerivedXMLParser110::VERSION[] = "1.1.0";
const char DerivedXMLParser110::SI_COMMON_URI[] = "urn:SICommon:1.0";

DerivedXMLParser110::DerivedXMLParser110(logging::Logger* log,
                                         bool ownLog) :
    DerivedXMLParser(VERSION,
                     std::auto_ptr<six::SICommonXMLParser>(
                         new six::SICommonXMLParser10x(versionToURI(VERSION),
                                                       false,
                                                       SI_COMMON_URI,
                                                       log)),
                     log,
                     ownLog)
{
}

DerivedData* DerivedXMLParser110::fromXML(
        const xml::lite::Document* doc) const
{
    XMLElem root = doc->getRootElement();

    XMLElem productCreationXML        = getFirstAndOnly(root, "ProductCreation");
    XMLElem displayXML                = getFirstAndOnly(root, "Display");
    XMLElem geographicAndTargetXML    = getFirstAndOnly(root, "GeographicAndTarget");
    XMLElem measurementXML            = getFirstAndOnly(root, "Measurement");
    XMLElem exploitationFeaturesXML   = getFirstAndOnly(root, "ExploitationFeatures");
    XMLElem productProcessingXML      = getOptional(root, "ProductProcessing");
    XMLElem downstreamReprocessingXML = getOptional(root, "DownstreamReprocessing");
    XMLElem errorStatisticsXML        = getOptional(root, "ErrorStatistics");
    XMLElem radiometricXML            = getOptional(root, "Radiometric");
    XMLElem matchInfoXML             = getOptional(root, "MatchInfo");
    XMLElem compressionXML            = getOptional(root, "Compression");
    XMLElem dedXML                    = getOptional(root, "DigitalElevationData");
    XMLElem annotationsXML            = getOptional(root, "Annotations");
    

    DerivedDataBuilder builder;
    DerivedData *data = builder.steal(); //steal it

    // see if PixelType has MONO or RGB
    PixelType pixelType = six::toType<PixelType>(
            getFirstAndOnly(displayXML, "PixelType")->getCharacterData());
    builder.addDisplay(pixelType);

    // create GeographicAndTarget
    builder.addGeographicAndTarget();

    // create Measurement
    six::ProjectionType projType = ProjectionType::NOT_SET;
    if (getOptional(measurementXML, "GeographicProjection"))
        projType = ProjectionType::GEOGRAPHIC;
    else if (getOptional(measurementXML, "CylindricalProjection"))
            projType = ProjectionType::CYLINDRICAL;
    else if (getOptional(measurementXML, "PlaneProjection"))
        projType = ProjectionType::PLANE;
    else if (getOptional(measurementXML, "PolynomialProjection"))
        projType = ProjectionType::POLYNOMIAL;
    builder.addMeasurement(projType);

    // create ExploitationFeatures
    std::vector<XMLElem> elements;
    exploitationFeaturesXML->getElementsByTagName("ExploitationFeatures",
                                                  elements);
    builder.addExploitationFeatures(elements.size());

    parseProductCreationFromXML(productCreationXML, data->productCreation.get());
    parseDisplayFromXML(displayXML, *data->display);
    parseGeographicTargetFromXML(geographicAndTargetXML, *data->geographicAndTarget);
    parseMeasurementFromXML(measurementXML, data->measurement.get());
    parseExploitationFeaturesFromXML(exploitationFeaturesXML, data->exploitationFeatures.get());

    if (productProcessingXML)
    {
        builder.addProductProcessing();
        parseProductProcessingFromXML(productProcessingXML,
                                      data->productProcessing.get());
    }
    if (downstreamReprocessingXML)
    {
        builder.addDownstreamReprocessing();
        parseDownstreamReprocessingFromXML(downstreamReprocessingXML,
                                           data->downstreamReprocessing.get());
    }
    if (errorStatisticsXML)
    {
        builder.addErrorStatistics();
        common().parseErrorStatisticsFromXML(errorStatisticsXML,
                                             data->errorStatistics.get());
    }
    if (radiometricXML)
    {
        builder.addRadiometric();
        common().parseRadiometryFromXML(radiometricXML,
                                        data->radiometric.get());
    }
    if (matchInfoXML)
    {
        builder.addMatchInformation();
        common().parseMatchInformationFromXML(matchInfoXML, data->matchInformation.get());
    }
    if (compressionXML)
    {
        builder.addCompression();
        parseCompressionFromXML(compressionXML, *data->compression);
    }
    if (dedXML)
    {
        builder.addDigitalElevationData();
        parseDigitalElevationDataFromXML(dedXML, *data->digitalElevationData);
    }
    if (annotationsXML)
    {
        // 1 to unbounded
        std::vector<XMLElem> annChildren;
        annotationsXML->getElementsByTagName("Annotation", annChildren);
        data->annotations.resize(annChildren.size());
        for (unsigned int i = 0, size = annChildren.size(); i < size; ++i)
        {
            data->annotations[i].reset(new Annotation());
            parseAnnotationFromXML(annChildren[i], data->annotations[i].get());
        }
    }
    return data;
}

xml::lite::Document* DerivedXMLParser110::toXML(const DerivedData* derived) const
{
    xml::lite::Document* doc = new xml::lite::Document();
    XMLElem root = newElement("SIDD");
    doc->setRootElement(root);

    convertProductCreationToXML(derived->productCreation.get(), root);
    convertDisplayToXML(*derived->display, root);

    convertGeographicTargetToXML(*derived->geographicAndTarget, root);
    convertMeasurementToXML(derived->measurement.get(), root);
    convertExploitationFeaturesToXML(derived->exploitationFeatures.get(),
                                     root);


    // optional
    if (derived->downstreamReprocessing.get())
    {
        convertDownstreamReprocessingToXML(
                derived->downstreamReprocessing.get(), root);
    }
    // optional
    if (derived->errorStatistics.get())
    {
        common().convertErrorStatisticsToXML(derived->errorStatistics.get(),
                                             root);
    }
    // optional
    if (derived->matchInformation.get())
    {
        common().convertMatchInformationToXML(*derived->matchInformation,
                                              root);
    }
    // optional
    if (derived->radiometric.get())
    {
        common().convertRadiometryToXML(derived->radiometric.get(), root);
    }
    // optional
    if (derived->compression.get())
    {
       convertCompressionToXML(*derived->compression, root);
    }
    // optional
    if (derived->digitalElevationData.get())
    {
        convertDigitalElevationDataToXML(*derived->digitalElevationData,
                                         root);
    }
    // optional
    if (derived->productProcessing.get())
    {
        convertProductProcessingToXML(derived->productProcessing.get(), root);
    }
    // optional
    if (!derived->annotations.empty())
    {
        XMLElem annotationsElem = newElement("Annotations", root);
        for (size_t i = 0, num = derived->annotations.size(); i < num; ++i)
        {
            convertAnnotationToXML(derived->annotations[i].get(),
                                   annotationsElem);
        }
    }

    //set the XMLNS
    root->setNamespacePrefix("", getDefaultURI());
    root->setNamespacePrefix("si", SI_COMMON_URI);
    root->setNamespacePrefix("sfa", SFA_URI);
    root->setNamespacePrefix("ism", ISM_URI);

    return doc;
}

void DerivedXMLParser110::parseDerivedClassificationFromXML(
        const XMLElem classificationXML,
        DerivedClassification& classification) const
{
    DerivedXMLParser::parseDerivedClassificationFromXML(classificationXML, classification);

    const XMLAttributes& classificationAttributes
        = classificationXML->getAttributes();

    getAttributeList(classificationAttributes,
        "ism:compliesWith",
        classification.compliesWith);

    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:exemptFrom",
        classification.exemptFrom);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:joint",
        classification.joint);
    // optional
    getAttributeListIfExists(classificationAttributes,
        "ism:atomicEnergyMarkings",
        classification.atomicEnergyMarkings);
    // optional
    getAttributeListIfExists(classificationAttributes,
        "ism:displayOnlyTo",
        classification.displayOnlyTo);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeType",
        classification.noticeType);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeReason",
        classification.noticeReason);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeDate",
        classification.noticeDate);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:unregisteredNoticeType",
        classification.unregisteredNoticeType);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:externalNotice",
        classification.externalNotice);
}

void DerivedXMLParser110::parseCompressionFromXML(const XMLElem compressionXML,
                                                 Compression& compression) const
{
    XMLElem j2kElem = getFirstAndOnly(compressionXML, "J2K");
    XMLElem originalElem = getFirstAndOnly(j2kElem, "Original");
    XMLElem parsedElem   = getOptional(j2kElem, "Parsed");

    parseJ2KCompression(originalElem, compression.original);
    if (parsedElem)
    {
        compression.parsed.reset(new J2KCompression());
        parseJ2KCompression(parsedElem, *compression.parsed);
    }
}

void DerivedXMLParser110::parseJ2KCompression(const XMLElem j2kXML,
                                              J2KCompression& j2k) const
{
    parseInt(getFirstAndOnly(j2kXML, "NumWaveletLevels"),
            j2k.numWaveletLevels);
    parseInt(getFirstAndOnly(j2kXML, "NumBands"),
            j2k.numBands);

    XMLElem layerInfoXML = getFirstAndOnly(j2kXML, "LayerInfo");
    std::vector<XMLElem> layersXML;
    layerInfoXML->getElementsByTagName("Layer", layersXML);

    size_t numLayers = layersXML.size();
    j2k.layerInfo.resize(numLayers);

    for (size_t ii = 0; ii < layersXML.size(); ++ii)
    {
        parseDouble(getFirstAndOnly(layersXML[ii], "Bitrate"),
                    j2k.layerInfo[ii].bitRate);
    }
}

void DerivedXMLParser110::parseDisplayFromXML(const XMLElem displayXML,
                                              Display& display) const
{
    //pixelType previously set

    parseBandInformationFromXML(getFirstAndOnly(displayXML, "BandInformation"),
                                *display.bandInformation);

    parseNonInteractiveProcessingFromXML(getFirstAndOnly(displayXML,
           "NonInteractiveProcessing"), *display.nonInteractiveProcessing);

    parseInteractiveProcessingFromXML(getFirstAndOnly(displayXML,
           "InteractiveProcessing"), *display.interactiveProcessing);

    std::vector<XMLElem> extensions;
    displayXML->getElementsByTagName("DisplayExtention", extensions);
    for (size_t ii = 0; ii < extensions.size(); ++ii)
    {
        std::string name;
        getAttributeIfExists(extensions[ii]->getAttributes(), "name", name);
        std::string value;
        parseString(extensions[ii], value);
        Parameter parameter(value);
        parameter.setName(name);
        display.displayExtensions.push_back(parameter);
    }
}

void DerivedXMLParser110::parseBandInformationFromXML(const XMLElem bandXML,
            BandInformation& bandInformation) const
{
    std::cerr << "Start band" << std::endl;
    std::vector<XMLElem> bands;
    bandXML->getElementsByTagName("Band", bands);
    bandInformation.bands.resize(bands.size());

    for (size_t ii = 0; ii < bands.size(); ++ii)
    {
        parseString(bands[ii], bandInformation.bands[ii]);
        parseInt(getFirstAndOnly(bandXML, "BitsPerPixel"),
                bandInformation.bitsPerPixel);
        parseInt(getFirstAndOnly(bandXML, "DisplayFlag"),
                 bandInformation.displayFlag);
    }
}

void DerivedXMLParser110::parseNonInteractiveProcessingFromXML(
            const XMLElem procElem,
            NonInteractiveProcessing& nonInteractiveProcessing) const
{
    XMLElem productGenerationOptions = getFirstAndOnly(procElem,
            "ProductGenerationOptions");
    XMLElem rrdsElem = getFirstAndOnly(procElem, "RRDS");

    parseProductGenerationOptionsFromXML(productGenerationOptions,
        nonInteractiveProcessing.productGenerationOptions);

    parseRRDSFromXML(rrdsElem, nonInteractiveProcessing.rrds);
}

void DerivedXMLParser110::parseProductGenerationOptionsFromXML(
            const XMLElem optionsElem,
            ProductGenerationOptions& options) const
{
    XMLElem bandElem = getOptional(optionsElem, "BandEqualization");
    XMLElem restoration = getFirstAndOnly(optionsElem,
            "ModularTransferFunctionRestoration");
    XMLElem remapElem = getOptional(optionsElem, "DataRemapping");
    XMLElem correctionElem = getOptional(optionsElem,
            "AsymmetricPixelCorrection");

    if (bandElem)
    {
        parseBandEqualizationFromXML(bandElem, *options.bandEqualization);
    }
    parseFilterFromXML(restoration, options.modularTransferFunctionRestoration);
    if (remapElem)
    {
        options.dataRemapping.reset(parseRemapChoiceFromXML(remapElem));
    }
    if (correctionElem)
    {
        parseFilterFromXML(correctionElem, *options.asymmetricPixelCorrection);
    }
}

void DerivedXMLParser110::parseBandEqualizationFromXML(const XMLElem bandElem,
                                                       BandEqualization& band) const
{
    std::string algorithmName;
    parseString(getFirstAndOnly(bandElem, "Algorithm"), algorithmName);
    band.algorithm = BandEqualizationAlgorithm(algorithmName);

    XMLElem LUTElem = getOptional(bandElem, "BandLUT");
    if (LUTElem)
    {
        band.bandLUT.reset(parseSingleLUT(LUTElem));
    }
}

void DerivedXMLParser110::parseRRDSFromXML(const XMLElem rrdsElem,
            RRDS& rrds) const
{
    std::string methodType;
    parseString(getFirstAndOnly(rrdsElem, "DownsamplingMethod"), methodType);
    rrds.downsamplingMethod = DownsamplingMethod(methodType);

    if (methodType != "DECIMATE")
    {
        return;
    }

    parseFilterFromXML(getFirstAndOnly(rrdsElem, "AntiAlias"), *rrds.antiAlias);
    parseFilterFromXML(getFirstAndOnly(rrdsElem, "Interpolation"),
            *rrds.interpolation);
}

void DerivedXMLParser110::parseFilterFromXML(const XMLElem FilterElem,
                                             Filter& Filter) const
{
    parseString(getFirstAndOnly(FilterElem, "FilterName"), Filter.FilterName);
    XMLElem customElem = getOptional(FilterElem, "Custom");
    XMLElem predefinedElem = getOptional(FilterElem, "Predefined");

    if (customElem)
    {
        std::string type;
        parseString(FilterElem, type);
        Filter.custom->type = FilterCustomType(type);
        XMLElem FilterSize = getFirstAndOnly(FilterElem, "FilterSize");
        parseInt(getFirstAndOnly(FilterSize, "Row"),
                Filter.custom->FilterSize.row);
        parseInt(getFirstAndOnly(FilterSize, "Col"),
                Filter.custom->FilterSize.col);

        XMLElem FilterCoef = getFirstAndOnly(FilterElem, "FilterCoef");
        std::vector<XMLElem> coefficients;
        FilterCoef->getElementsByTagName("Coef", coefficients);
        size_t numCoefs = coefficients.size();
        Filter.custom->FilterCoef.resize(numCoefs);
        for (size_t ii = 0; ii < numCoefs; ++ii)
        {
            parseDouble(coefficients[ii], Filter.custom->FilterCoef[ii]);
        }
    }
    else if (predefinedElem)
    {
        bool ok = false;
        XMLElem dbNameElem = getOptional(predefinedElem, "DBName");
        XMLElem familyElem = getOptional(predefinedElem, "FilterFamily");
        XMLElem FilterMember = getOptional(predefinedElem, "FilterMember");

        if (dbNameElem)
        {
            if (!familyElem && !FilterMember)
            {
                ok = true;

                std::string name;
                parseString(dbNameElem, name);
                Filter.predefined->dbName = FilterDatabaseName(name);
            }
        }
        else if (familyElem && FilterMember)
        {
            ok = true;

            parseInt(familyElem, Filter.predefined->FilterFamily);
            parseInt(familyElem, Filter.predefined->FilterMember);
        }
        if (!ok)
        {
            throw except::Exception(Ctxt(
                    "Exactly one of either dbName or FilterFamily and "
                    "FilterMember must be defined"));
        }
    }
    std::string opName;
    parseString(getFirstAndOnly(FilterElem, "Operation"), opName);
    Filter.operation = FilterOperation(opName);
}

void DerivedXMLParser110::parseInteractiveProcessingFromXML(
            const XMLElem interactiveElem,
            InteractiveProcessing& interactive) const
{
    XMLElem geomElem = getFirstAndOnly(interactiveElem, "GeometricTransform");
    XMLElem sharpnessElem = getFirstAndOnly(interactiveElem,
            "SharpnesEnhancement");
    XMLElem colorElem = getOptional(interactiveElem, "ColorSpaceTransform");
    XMLElem dynamicElem = getOptional(interactiveElem, "DynamicRangeAdjustment");
    XMLElem ttcElem = getOptional(interactiveElem, "TonalTransferCurve");

    parseGeometricTransformFromXML(geomElem, interactive.geometricTransform);
    parseSharpnessEnhancementFromXML(sharpnessElem,
                                     interactive.sharpnessEnhancement);
    if (colorElem)
    {
        parseColorSpaceTransformFromXML(colorElem,
                                        *interactive.colorSpaceTransform);
    }
    if (dynamicElem)
    {
        parseDynamicRangeAdjustmentFromXML(dynamicElem,
                                           *interactive.dynamicRangeAdjustment);
    }
    if (ttcElem)
    {
        interactive.tonalTransferCurve.reset(parseSingleLUT(ttcElem));
    }
}

void DerivedXMLParser110::parseGeometricTransformFromXML(const XMLElem geomElem,
             GeometricTransform& transform) const
{
    XMLElem scalingElem = getFirstAndOnly(geomElem, "Scaling");
    parseFilterFromXML(getFirstAndOnly(scalingElem, "AntiAlias"),
        transform.scaling.antiAlias);
    parseFilterFromXML(getFirstAndOnly(scalingElem, "Interpolation"),
        transform.scaling.interpolation);

    XMLElem orientationElem = getFirstAndOnly(geomElem, "Orientation");
    std::string shadowDirection;
    parseString(getFirstAndOnly(orientationElem, "ShadowDirection"), shadowDirection);
    transform.orientation.shadowDirection = ShadowDirection(shadowDirection);
}

void DerivedXMLParser110::parseSharpnessEnhancementFromXML(
             const XMLElem sharpElem,
             SharpnessEnhancement& sharpness) const
{
    bool ok = false;
    XMLElem mTFCElem = getOptional(sharpElem,
                                   "ModularTransferFunctionCompensation");
    XMLElem mTFRElem = getOptional(sharpElem,
                                   "ModularTransferFunctionRestoration");
    if (mTFCElem)
    {
        if (!mTFRElem)
        {
            ok = true;
            parseFilterFromXML(mTFCElem,
                               *sharpness.modularTransferFunctionCompensation);
        }
    }
    else if (mTFRElem)
    {
        ok = true;
        parseFilterFromXML(mTFRElem,
                           *sharpness.modularTransferFunctionRestoration);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of modularTransferFunctionCompensation or "
                "modularTransferFunctionRestoration must be set"));
    }
}

void DerivedXMLParser110::parseColorSpaceTransformFromXML(
            const XMLElem colorElem, ColorSpaceTransform& transform) const
{
    XMLElem manageElem = getFirstAndOnly(colorElem, "ColorManagementModule");
    std::string intentName;
    parseString(getFirstAndOnly(manageElem, "RenderingIntent"), intentName);
    transform.colorManagementModule.renderingIntent =
            RenderingIntent(intentName);
    parseString(getFirstAndOnly(manageElem, "SourceProfile"),
                transform.colorManagementModule.sourceProfile);
    parseString(getFirstAndOnly(manageElem, "DisplayProfile"),
                transform.colorManagementModule.displayProfile);
    parseString(getFirstAndOnly(manageElem, "ICCProfile"),
                transform.colorManagementModule.iccProfile);
}

void DerivedXMLParser110::parseDynamicRangeAdjustmentFromXML(
            const XMLElem rangeElem,
            DynamicRangeAdjustment& rangeAdjustment) const
{
    std::string algTypeName;
    parseString(getFirstAndOnly(rangeElem, "AlgorithmType"), algTypeName);
    rangeAdjustment.algorithmType = DRAType(algTypeName);

    bool ok = false;
    XMLElem parameterElem = getOptional(rangeElem, "DRAParameters");
    XMLElem overrideElem = getOptional(rangeElem, "DRAOverrides");
    if (parameterElem)
    {
        if (!overrideElem)
        {
            ok = true;

            parseDouble(getFirstAndOnly(parameterElem, "Pmin"), rangeAdjustment.draParameters->pMin);
            parseDouble(getFirstAndOnly(parameterElem, "Pmax"), rangeAdjustment.draParameters->pMax);
            parseDouble(getFirstAndOnly(parameterElem, "EminModifier"), rangeAdjustment.draParameters->eMinModifier);
            parseDouble(getFirstAndOnly(parameterElem, "EmaxModifier"), rangeAdjustment.draParameters->eMaxModifier);
        }
    }
    else if (overrideElem)
    {
        ok = true;
        parseDouble(getFirstAndOnly(overrideElem, "Subtractor"),
            rangeAdjustment.draOverrides->subtractor);
        parseDouble(getFirstAndOnly(overrideElem, "Multiplier"),
            rangeAdjustment.draOverrides->multiplier);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("XML should have exactly one of DRAParameters and DRAOverrides"));
    }
}

XMLElem DerivedXMLParser110::convertDerivedClassificationToXML(
        const DerivedClassification& classification,
        XMLElem parent) const
{
    XMLElem classXML = newElement("Classification", parent);

    common().addParameters("SecurityExtension",
                    classification.securityExtensions,
                           classXML);

    //! from ism:ISMRootNodeAttributeGroup
    // SIDD 1.1 is tied to IC-ISM v13
    setAttribute(classXML, "DESVersion", "13", ISM_URI);

    // So far as I can tell this should just be 1
    setAttribute(classXML, "ISMCATCESVersion", "1", ISM_URI);

    //! from ism:ResourceNodeAttributeGroup
    setAttribute(classXML, "resourceElement", "true", ISM_URI);
    setAttribute(classXML, "createDate",
                 classification.createDate.format("%Y-%m-%d"), ISM_URI);
    // required (was optional in SIDD 1.0)
    setAttributeList(classXML, "compliesWith", classification.compliesWith,
                     ISM_URI);

    // optional
    setAttributeIfNonEmpty(classXML,
                           "exemptFrom",
                           classification.exemptFrom,
                           ISM_URI);

    //! from ism:SecurityAttributesGroup
    //  -- referenced in ism::ResourceNodeAttributeGroup
    setAttribute(classXML, "classification", classification.classification,
                 ISM_URI);
    setAttributeList(classXML, "ownerProducer", classification.ownerProducer,
                     ISM_URI, true);

    // optional
    setAttributeIfNonEmpty(classXML, "joint", classification.joint, ISM_URI);

    // optional
    setAttributeList(classXML, "SCIcontrols", classification.sciControls,
                     ISM_URI);
    // optional
    setAttributeList(classXML, "SARIdentifier", classification.sarIdentifier,
                     ISM_URI);
    // optional
    setAttributeList(classXML,
                     "atomicEnergyMarkings",
                     classification.atomicEnergyMarkings,
                     ISM_URI);
    // optional
    setAttributeList(classXML,
                     "disseminationControls",
                     classification.disseminationControls,
                     ISM_URI);
    // optional
    setAttributeList(classXML,
                     "displayOnlyTo",
                     classification.displayOnlyTo,
                     ISM_URI);
    // optional
    setAttributeList(classXML, "FGIsourceOpen", classification.fgiSourceOpen,
                     ISM_URI);
    // optional
    setAttributeList(classXML,
                     "FGIsourceProtected",
                     classification.fgiSourceProtected,
                     ISM_URI);
    // optional
    setAttributeList(classXML, "releasableTo", classification.releasableTo,
                     ISM_URI);
    // optional
    setAttributeList(classXML, "nonICmarkings", classification.nonICMarkings,
                     ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "classifiedBy",
                           classification.classifiedBy,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "compilationReason",
                           classification.compilationReason,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "derivativelyClassifiedBy",
                           classification.derivativelyClassifiedBy,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "classificationReason",
                           classification.classificationReason,
                           ISM_URI);
    // optional
    setAttributeList(classXML, "nonUSControls", classification.nonUSControls,
                     ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "derivedFrom",
                           classification.derivedFrom,
                           ISM_URI);
    // optional
    if (classification.declassDate.get())
    {
        setAttributeIfNonEmpty(
                classXML, "declassDate",
                classification.declassDate->format("%Y-%m-%d"),
                ISM_URI);
    }
    // optional
    setAttributeIfNonEmpty(classXML,
                           "declassEvent",
                           classification.declassEvent,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "declassException",
                           classification.declassException,
                           ISM_URI);

    //! from ism:NoticeAttributesGroup
    // optional
    setAttributeIfNonEmpty(classXML,
                           "noticeType",
                           classification.noticeType,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "noticeReason",
                           classification.noticeReason,
                           ISM_URI);
    // optional
    if (classification.noticeDate.get())
    {
        setAttributeIfNonEmpty(
                classXML, "noticeDate",
                classification.noticeDate->format("%Y-%m-%d"),
                ISM_URI);
    }
    // optional
    setAttributeIfNonEmpty(classXML,
                           "unregisteredNoticeType",
                           classification.unregisteredNoticeType,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classXML,
                           "externalNotice",
                           classification.externalNotice,
                           ISM_URI);

    return classXML;
}

XMLElem DerivedXMLParser110::convertNonInteractiveProcessingToXML(
        const NonInteractiveProcessing& processing,
        XMLElem parent) const
{
    XMLElem processingXML = newElement("NonInteractiveProcessing", parent);

    // ProductGenerationOptions
    XMLElem prodGenXML = newElement("ProductGenerationOptions",
                                    processingXML);

    const ProductGenerationOptions& prodGen =
            processing.productGenerationOptions;

    if (prodGen.bandEqualization.get())
    {
        const BandEqualization& bandEq = *prodGen.bandEqualization;

        XMLElem bandEqXML = newElement("BandEqualization", prodGenXML);
        createStringFromEnum("Algorithm", bandEq.algorithm, bandEqXML);
        createLUT("BandLUT", bandEq.bandLUT.get(), bandEqXML);
    }

    convertFilterToXML("ModularTransferFunctionRestoration",
                       prodGen.modularTransferFunctionRestoration,
                       prodGenXML);

    if (prodGen.dataRemapping.get())
    {
        XMLElem dataRemappingXML = newElement("DataRemapping", prodGenXML);
        convertRemapToXML(*prodGen.dataRemapping, dataRemappingXML);
    }

    if (prodGen.asymmetricPixelCorrection.get())
    {
        convertFilterToXML("AsymmetricPixelCorrection",
                           *prodGen.asymmetricPixelCorrection, prodGenXML);
    }

    // RRDS
    XMLElem rrdsXML = newElement("RRDS", processingXML);

    const RRDS& rrds = processing.rrds;
    createStringFromEnum("DownsamplingMethod", rrds.downsamplingMethod,
                         rrdsXML);

    if (rrds.downsamplingMethod == DownsamplingMethod::DECIMATE)
    {
        confirmNonNull(rrds.antiAlias, "antiAlias",
                       "for DECIMATE downsampling");
        convertFilterToXML("AntiAlias", *rrds.antiAlias, rrdsXML);

        confirmNonNull(rrds.interpolation, "interpolation",
                       "for DECIMATE downsampling");
        convertFilterToXML("Interpolation", *rrds.interpolation, rrdsXML);
    }

    return processingXML;
}

XMLElem DerivedXMLParser110::convertInteractiveProcessingToXML(
        const InteractiveProcessing& processing,
        XMLElem parent) const
{
    XMLElem processingXML = newElement("InteractiveProcessing", parent);

    // GeometricTransform
    const GeometricTransform& geoTransform(processing.geometricTransform);
    XMLElem geoTransformXML = newElement("GeometricTransform", processingXML);

    XMLElem scalingXML = newElement("Scaling", geoTransformXML);
    convertFilterToXML("AntiAlias", geoTransform.scaling.antiAlias,
                       scalingXML);
    convertFilterToXML("Interpolation", geoTransform.scaling.interpolation,
                       scalingXML);

    XMLElem orientationXML = newElement("Orientation", geoTransformXML);
    createStringFromEnum("ShadowDirection",
        geoTransform.orientation.shadowDirection,
        orientationXML);

    // SharpnessEnhancement
    const SharpnessEnhancement& sharpness(processing.sharpnessEnhancement);
    XMLElem sharpXML = newElement("SharpnessEnhancement", processingXML);

    bool ok = false;
    if (sharpness.modularTransferFunctionCompensation.get())
    {
        if (sharpness.modularTransferFunctionRestoration.get() == NULL)
        {
            ok = true;
            convertFilterToXML("ModularTransferFunctionCompensation",
                               *sharpness.modularTransferFunctionCompensation,
                               sharpXML);
        }
    }
    else if (sharpness.modularTransferFunctionRestoration.get())
    {
        ok = true;
        convertFilterToXML("ModularTransferFunctionRestoration",
                           *sharpness.modularTransferFunctionRestoration,
                           sharpXML);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of modularTransferFunctionCompensation or "
                "modularTransferFunctionRestoration must be set"));
    }

    // ColorSpaceTransform
    if (processing.colorSpaceTransform.get())
    {
        const ColorManagementModule& cmm =
                processing.colorSpaceTransform->colorManagementModule;

        XMLElem colorSpaceTransformXML =
                newElement("ColorSpaceTransform", processingXML);
        XMLElem cmmXML =
                newElement("ColorManagementModule", colorSpaceTransformXML);

        createStringFromEnum("RenderingIntent", cmm.renderingIntent, cmmXML);

        // TODO: Not sure what this'll actually look like
        createString("SourceProfile", cmm.sourceProfile, cmmXML);
        createString("DisplayProfile", cmm.displayProfile, cmmXML);

        if (!cmm.iccProfile.empty())
        {
            createString("ICCProfile", cmm.iccProfile, cmmXML);
        }
    }

    // DynamicRangeAdjustment
    if (processing.dynamicRangeAdjustment.get())
    {
        const DynamicRangeAdjustment& adjust =
                *processing.dynamicRangeAdjustment;

        XMLElem adjustXML =
                newElement("DynamicRangeAdjustment", processingXML);

        createStringFromEnum("AlgorithmType", adjust.algorithmType,
                             adjustXML);

        bool ok = false;
        if (adjust.draParameters.get())
        {
            if (!adjust.draOverrides.get())
            {
                ok = true;
                XMLElem paramXML = newElement("DRAParameters", adjustXML);
                createDouble("Pmin", adjust.draParameters->pMin, paramXML);
                createDouble("Pmax", adjust.draParameters->pMax, paramXML);
                createDouble("EminModifier", adjust.draParameters->eMinModifier, paramXML);
                createDouble("EmaxModifier", adjust.draParameters->eMinModifier, paramXML);
            }
        }
        else if (adjust.draOverrides.get())
        {
            ok = true;
            XMLElem overrideXML = newElement("DRAOverrides", adjustXML);
            createDouble("Subtractor", adjust.draOverrides->subtractor, overrideXML);
            createDouble("Multiplier", adjust.draOverrides->multiplier, overrideXML);
        }
        if (!ok)
        {
            throw except::Exception(Ctxt("Data must contain exactly one of DRAParameters and DRAOverrides"));
        }
    }

    if (processing.tonalTransferCurve.get())
    {
        createLUT("TonalTransferCurve", processing.tonalTransferCurve.get(), processingXML);
    }

    return processingXML;
}

XMLElem DerivedXMLParser110::convertPredefinedFilterToXML(
        const Filter::Predefined& predefined,
        XMLElem parent) const
{
    XMLElem predefinedXML = newElement("Predefined", parent);

    // Make sure either DBName or FilterFamily+FilterMember are defined
    bool ok = false;
    if (isDefined(predefined.dbName))
    {
        if (six::Init::isUndefined(predefined.FilterFamily) &&
            six::Init::isUndefined(predefined.FilterMember))
        {
            ok = true;

            createStringFromEnum("DBName", predefined.dbName, predefinedXML);
        }
    }
    else if (six::Init::isDefined(predefined.FilterFamily) &&
             six::Init::isDefined(predefined.FilterMember))
    {
        ok = true;

        createInt("FilterFamily", predefined.FilterFamily, predefinedXML);
        createInt("FilterMember", predefined.FilterMember, predefinedXML);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of either dbName or FilterFamily and "
                "FilterMember must be defined"));
    }

    return predefinedXML;
}

XMLElem DerivedXMLParser110::convertCustomFilterToXML(
        const Filter::Custom& custom,
        XMLElem parent) const
{
    XMLElem customXML = newElement("Custom", parent);

    //XMLElem FilterSize = newElement("FilterSize", customXML);
    //createInt("Row", custom.FilterSize.row, FilterSize);
    //createInt("Col", custom.FilterSize.col, FilterSize);
    common().createRowCol("FilterSize", "Row", "Col", custom.FilterSize, customXML);

    if (custom.FilterCoef.size() !=
        static_cast<size_t>(custom.FilterSize.row) * custom.FilterSize.col)
    {
        std::ostringstream ostr;
        ostr << "Filter size is " << custom.FilterSize.row << " rows x "
             << custom.FilterSize.col << " cols but have "
             << custom.FilterCoef.size() << " coefficients";
        throw except::Exception(Ctxt(ostr.str()));
    }

    XMLElem FilterCoef = newElement("FilterCoef", customXML);
    for (sys::SSize_T row = 0, idx = 0; row < custom.FilterSize.row; ++row)
    {
        for (sys::SSize_T col = 0; col < custom.FilterSize.col; ++col, ++idx)
        {
            XMLElem coefXML = createDouble("Coef", custom.FilterCoef[idx],
                                           FilterCoef);
            setAttribute(coefXML, "row", str::toString(row));
            setAttribute(coefXML, "col", str::toString(col));
        }
    }

    return customXML;
}

XMLElem DerivedXMLParser110::convertFilterToXML(const std::string& name,
                                                const Filter& Filter,
                                                XMLElem parent) const
{
    XMLElem FilterXML = newElement(name, parent);

    createString("FilterName", Filter.FilterName, FilterXML);

    // Exactly one of Predefined or Custom should be set
    bool ok = false;
    if (Filter.predefined.get())
    {
        if (Filter.custom.get() == NULL)
        {
            ok = true;
            convertPredefinedFilterToXML(*Filter.predefined, FilterXML);
        }
    }
    else if (Filter.custom.get())
    {
        ok = true;
        convertCustomFilterToXML(*Filter.custom, FilterXML);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of predefined or custom must be set"));
    }

    createStringFromEnum("Operation", Filter.operation, FilterXML);

    return FilterXML;
}

XMLElem DerivedXMLParser110::convertCompressionToXML(
        const Compression& compression,
        XMLElem parent) const
{
    XMLElem compressionXML = newElement("Compression", parent);
    XMLElem j2kXML = newElement("J2K", compressionXML);
    XMLElem originalXML = newElement("Original", j2kXML);
    convertJ2KToXML(compression.original, originalXML);

    if (compression.parsed.get())
    {
        XMLElem parsedXML = newElement("Parsed", j2kXML);
        convertJ2KToXML(*compression.parsed, parsedXML);
    }
    return compressionXML;
}

void DerivedXMLParser110::convertJ2KToXML(const J2KCompression& j2k,
                                          XMLElem& parent) const
{
    createInt("NumWaveletLevels", j2k.numWaveletLevels, parent);
    createInt("NumBands", j2k.numBands, parent);

    size_t numLayers = j2k.layerInfo.size();
    XMLElem layerInfoXML = newElement("LayerInfo", parent);
    setAttribute(layerInfoXML, "numLayers", toString(numLayers));

    for (size_t ii = 0; ii < numLayers; ++ii)
    {
        XMLElem layerXML = newElement("Layer", layerInfoXML);
        setAttribute(layerXML, "index", toString(ii));
        createDouble("Bitrate", j2k.layerInfo[ii].bitRate, layerXML);
    }
}

XMLElem DerivedXMLParser110::convertDisplayToXML(
        const Display& display,
        XMLElem parent) const
{
    // NOTE: In several spots here, there are fields which are required in
    //       SIDD 1.1 but a pointer in the Display class since it didn't exist
    //       in SIDD 1.0, so need to confirm it's allocated

    XMLElem displayXML = newElement("Display", parent);

    createString("PixelType", six::toString(display.pixelType), displayXML);

    // BandInformation
    XMLElem bandInfoXML = newElement("BandInformation", displayXML);
    confirmNonNull(display.bandInformation, "bandInformation");
    createInt("NumBands", display.bandInformation->bands.size(), bandInfoXML);
    for (size_t ii = 0; ii < display.bandInformation->bands.size(); ++ii)
    {
        XMLElem bandXML = createString("Band",
                                       display.bandInformation->bands[ii],
                                       bandInfoXML);
        setAttribute(bandXML, "index", str::toString(ii));
    }
    createInt("BitsPerPixel", display.bandInformation->bitsPerPixel,
              bandInfoXML);
    createInt("DisplayFlag", display.bandInformation->displayFlag,
              bandInfoXML);

    // NonInteractiveProcessing
    confirmNonNull(display.nonInteractiveProcessing,
                   "nonInteractiveProcessing");
    convertNonInteractiveProcessingToXML(*display.nonInteractiveProcessing,
                                         displayXML);

    // InteractiveProcessing
    confirmNonNull(display.interactiveProcessing, "interactiveProcessing");
    convertInteractiveProcessingToXML(*display.interactiveProcessing,
                                      displayXML);

    // optional to unbounded
    common().addParameters("DisplayExtension", display.displayExtensions,
                           displayXML);

    return displayXML;
}

XMLElem DerivedXMLParser110::convertGeographicTargetToXML(
        const GeographicAndTarget& geographicAndTarget,
        XMLElem parent) const
{
    XMLElem geographicAndTargetXML = newElement("GeographicAndTarget", parent);

    createStringFromEnum("EarthModel",
        geographicAndTarget.earthModel,
        geographicAndTargetXML);

    confirmNonNull(geographicAndTarget.imageCorners,
                   "geographicAndTarget.imageCorners");
    common().createLatLonFootprint("ImageCorners", "ICP",
                                   *geographicAndTarget.imageCorners,
                                   geographicAndTargetXML);

    //only if 3+ vertices
    const size_t numVertices = geographicAndTarget.validData.size();
    if (numVertices >= 3)
    {
        XMLElem vXML = newElement("ValidData", geographicAndTargetXML);
        setAttribute(vXML, "size", str::toString(numVertices));

        for (size_t ii = 0; ii < numVertices; ++ii)
        {
            XMLElem vertexXML =
                    common().createLatLon("Vertex",
                                          geographicAndTarget.validData[ii],
                                          vXML);
            setAttribute(vertexXML, "index", str::toString(ii + 1));
        }
    }

    for (size_t ii = 0; ii < geographicAndTarget.geoInfos.size(); ++ii)
    {
        common().convertGeoInfoToXML(*geographicAndTarget.geoInfos[ii],
                                     geographicAndTargetXML);
    }

    return geographicAndTargetXML;
}

XMLElem DerivedXMLParser110::convertDigitalElevationDataToXML(
        const DigitalElevationData& ded,
        XMLElem parent) const
{
    XMLElem dedXML = newElement("DigitalElevationData", parent);

    // GeographicCoordinates
    XMLElem geoCoordXML = newElement("GeographicCoordinates", dedXML);
    createDouble("LongitudeDensity",
                 ded.geographicCoordinates.longitudeDensity,
                 geoCoordXML);
    createDouble("LatitudeDensity",
                 ded.geographicCoordinates.latitudeDensity,
                 geoCoordXML);
    common().createLatLon("ReferenceOrigin",
                          ded.geographicCoordinates.referenceOrigin,
                          geoCoordXML);

    // Geopositioning
    XMLElem geoposXML = newElement("Geopositioning", dedXML);
    createStringFromEnum("CoordinateSystemType",
                         ded.geopositioning.coordinateSystemType,
                         geoposXML);
    createString("GeodeticDatum", ded.geopositioning.geodeticDatum,
                 geoposXML);
    createString("ReferenceEllipsoid", ded.geopositioning.referenceEllipsoid,
                 geoposXML);
    createString("VerticalDatum", ded.geopositioning.verticalDatum,
                 geoposXML);
    createString("SoundingDatum", ded.geopositioning.soundingDatum,
                 geoposXML);
    createInt("FalseOrigin", ded.geopositioning.falseOrigin, geoposXML);
    if (ded.geopositioning.coordinateSystemType == CoordinateSystemType::UTM)
    {
        createInt("UTMGridZoneNumber",
                  ded.geopositioning.utmGridZoneNumber,
                  geoposXML);
    }

    // PositionalAccuracy
    XMLElem posAccXML = newElement("PositionalAccuracy", dedXML);
    createInt("NumRegions", ded.positionalAccuracy.numRegions, posAccXML);

    XMLElem absAccXML = newElement("AbsoluteAccuracy", posAccXML);
    createDouble("Horizontal",
                 ded.positionalAccuracy.absoluteAccuracyHorizontal,
                 absAccXML);
    createDouble("Vertical",
                 ded.positionalAccuracy.absoluteAccuracyVertical,
                 absAccXML);

    XMLElem p2pAccXML = newElement("PointToPointAccuracy", posAccXML);
    createDouble("Horizontal",
                 ded.positionalAccuracy.pointToPointAccuracyHorizontal,
                 p2pAccXML);
    createDouble("Vertical",
                 ded.positionalAccuracy.pointToPointAccuracyVertical,
                 p2pAccXML);

    if (six::Init::isDefined(ded.nullValue))
    {
        createInt("NullValue", ded.nullValue, dedXML);
    }

    return dedXML;
}

void DerivedXMLParser110::parseGeographicTargetFromXML(
    const XMLElem geographicElem,
    GeographicAndTarget& geographicAndTarget) const
{
    std::string model;
    parseString(getFirstAndOnly(geographicElem, "EarthModel"), model);
    geographicAndTarget.earthModel = EarthModelType(model);
    common().parseFootprint(getFirstAndOnly(geographicElem, "ImageCorners"), "ICP",
        *geographicAndTarget.imageCorners);

    XMLElem dataElem = getOptional(geographicElem, "ValidData");
    if (dataElem)
    {
        common().parseLatLons(dataElem, "Vertex", geographicAndTarget.validData);
    }

    std::vector<XMLElem> geoInfosXML;
    geographicElem->getElementsByTagName("GeoInfo", geoInfosXML);

    //optional
    size_t idx(geographicAndTarget.geoInfos.size());
    geographicAndTarget.geoInfos.resize(idx + geoInfosXML.size());

    for (std::vector<XMLElem>::const_iterator it = geoInfosXML.begin(); it
        != geoInfosXML.end(); ++it, ++idx)
    {
        geographicAndTarget.geoInfos[idx].reset(new GeoInfo());
        common().parseGeoInfoFromXML(*it, geographicAndTarget.geoInfos[idx].get());
    }
}

void DerivedXMLParser110::parseDigitalElevationDataFromXML(
        const XMLElem elem,
        DigitalElevationData& ded) const
{
    XMLElem coordXML = getFirstAndOnly(elem, "GeographicCoordinates");
    parseDouble(getFirstAndOnly(coordXML, "LongitudeDensity"), ded.geographicCoordinates.longitudeDensity);
    parseDouble(getFirstAndOnly(coordXML, "LatitudeDensity"), ded.geographicCoordinates.latitudeDensity);
    common().parseLatLon(getFirstAndOnly(coordXML, "ReferenceOrigin"), ded.geographicCoordinates.referenceOrigin);

    XMLElem posXML = getFirstAndOnly(elem, "Geopositioning");
    std::string coordSystemType;
    parseString(getFirstAndOnly(posXML, "CoordinateSystemType"), coordSystemType);
    ded.geopositioning.coordinateSystemType = CoordinateSystemType(coordSystemType);
    parseUInt(getFirstAndOnly(posXML, "FalseOrigin"), ded.geopositioning.falseOrigin);
    parseInt(getFirstAndOnly(posXML, "UTMGridZoneNumber"), ded.geopositioning.utmGridZoneNumber);

    XMLElem posAccuracyXML = getFirstAndOnly(elem, "PositionalAccuracy");
    parseUInt(getFirstAndOnly(posAccuracyXML, "PositionalAccuracyRegions"), ded.positionalAccuracy.numRegions);
    XMLElem absoluteXML = getFirstAndOnly(posAccuracyXML, "AbsoluteAccuracy");
    parseDouble(getFirstAndOnly(absoluteXML, "Horizontal"), ded.positionalAccuracy.absoluteAccuracyHorizontal);
    parseDouble(getFirstAndOnly(absoluteXML, "Vertical"), ded.positionalAccuracy.absoluteAccuracyVertical);
    XMLElem pointXML = getFirstAndOnly(posAccuracyXML, "PointToPointAccuracy");
    parseDouble(getFirstAndOnly(pointXML, "Horizontal"), ded.positionalAccuracy.pointToPointAccuracyHorizontal);
    parseDouble(getFirstAndOnly(pointXML, "Vertical"), ded.positionalAccuracy.pointToPointAccuracyVertical);
}
}
}
