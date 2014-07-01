#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "ProcessBySample.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

#include <fstream>
#include <iostream>
#include <QString>

using namespace std;
using namespace Isis;

void flip(Buffer &in);


void IsisMain() {

  // NOTE: 
  // Still to be done/considered
  //   Process the other FITS channels. One of them contains pixel quality info
  //   May want to set special pixel values using this channel

  UserInterface &ui = Application::GetUserInterface();

  ProcessImportFits importFits;

  importFits.setFitsFile(FileName(ui.GetFileName("FROM")));

  // Get the first label and make sure this is a New Horizons LORRI file
  PvlGroup mainLabel = importFits.fitsLabel(0);
  if (mainLabel["MISSION"][0] != "New Horizons" || mainLabel["INSTRU"][0] != "lor") {
    QString msg = QObject::tr("Input file [%1] does not appear to be a New Horizons LORRI FITS "
                              "file. Input file label value for MISSION is [%2], "
                              "INSTRU is [%3]").
                  arg(ui.GetFileName("FROM")).arg(mainLabel["MISSION"][0]).
                  arg(mainLabel["INSTRU"][0]);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  importFits.setProcessFileStructure(0);

  Cube *output = importFits.SetOutputCube("TO");

  // Get the directory where the New Horizons translation tables are.
  PvlGroup dataDir(Preference::Preferences().findGroup("DataDirectory"));
  QString transDir = (QString) dataDir["NewHorizons"] + "/translations/";

  // Temp storage of translated labels
  Pvl outLabel;

  // Get the FITS label
  Pvl fitsLabel;
  fitsLabel.addGroup(importFits.fitsLabel(0));

  // Create an Instrument group
  FileName insTransFile(transDir + "lorriInstrument_fit.trn");
  PvlTranslationManager insXlater(fitsLabel, insTransFile.expanded());
  insXlater.Auto(outLabel);

  // Modify/add Instument group keywords not handled by the translater
  PvlGroup &inst = outLabel.findGroup("Instrument", Pvl::Traverse);
  QString target = (QString)inst["TargetName"];
  if (target.startsWith("RADEC=")) {
    inst.addKeyword(PvlKeyword("TargetName", "Sky"), PvlGroup::Replace);
  }

  output->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));

  // Create a Band Bin group
  FileName bandTransFile(transDir + "lorriBandBin_fit.trn");
  PvlTranslationManager bandBinXlater(fitsLabel, bandTransFile.expanded());
  bandBinXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

  // Create an Archive group
  FileName archiveTransFile(transDir + "lorriArchive_fit.trn");
  PvlTranslationManager archiveXlater(fitsLabel, archiveTransFile.expanded());
  archiveXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));

  // Create a Kernels group
  FileName kernelsTransFile(transDir + "lorriKernels_fit.trn");
  PvlTranslationManager kernelsXlater(fitsLabel, kernelsTransFile.expanded());
  kernelsXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));

  // Save the input FITS label in the Cube original labels
  Pvl pvl;
  pvl += importFits.fitsLabel(0);
  OriginalLabel originals(pvl);
  output->write(originals);

  // Convert the main image data
  importFits.Progress()->SetText("Importing main LORRI image");
  importFits.StartProcess();
  importFits.ClearCubes();


  // Convert the Error image. It is currently assumed to be the second image in the FITS file
  if (ui.WasEntered("ERROR")) {
    // Get the label of the Error image and make sure this is a New Horizons LORRI Error image
    PvlGroup errorLabel = importFits.fitsLabel(1);
    if (errorLabel["XTENSION"][0] != "IMAGE" || errorLabel["EXTNAME"][0] != "LORRI Error image") {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a LORRI Error image "
                                "Input file label value for EXTNAME is [%2]").
                    arg(ui.GetFileName("FROM")).arg(mainLabel["EXTNAME"][0]);
      throw IException(IException::User, msg, _FILEINFO_);
    }

    importFits.setProcessFileStructure(1);

    Cube *outputError = importFits.SetOutputCube("ERROR");

    // Save the input FITS label in the Cube original labels
    Pvl pvlError;
    pvlError += importFits.fitsLabel(1);
    OriginalLabel originals(pvlError);
    outputError->write(originals);

    // Convert the main image data
    importFits.Progress()->SetText("Importing LORRI Error image");
    importFits.StartProcess();
    importFits.ClearCubes();
  }


  // Convert the Quality image. It is currently assumed to be the third image in the FITS file
  if (ui.WasEntered("QUALITY")) {
    // Get the label of the Quality image and make sure this is a New Horizons LORRI Quality image
    PvlGroup errorLabel = importFits.fitsLabel(2);
    if (errorLabel["XTENSION"][0] != "IMAGE" || 
        errorLabel["EXTNAME"][0] != "LORRI Quality flag image") {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a LORRI Quality image "
                                "Input file label value for EXTNAME is [%2]").
                    arg(ui.GetFileName("FROM")).arg(mainLabel["EXTNAME"][0]);
      throw IException(IException::User, msg, _FILEINFO_);
    }

    importFits.setProcessFileStructure(2);

    Cube *outputError = importFits.SetOutputCube("QUALITY");

    // Save the input FITS label in the Cube original labels
    Pvl pvlError;
    pvlError += importFits.fitsLabel(2);
    OriginalLabel originals(pvlError);
    outputError->write(originals);

    // Convert the main image data
    importFits.Progress()->SetText("Importing LORRI Quality image");
    importFits.StartProcess();
    importFits.ClearCubes();
  }

  importFits.Finalize();

// The images need to be flipped from top to bottom to put the origin in the upper left for ISIS
// Commented out because we don't need to do this. If we find later that we do, remove comments
//  ProcessBySample flipLines;
//  CubeAttributeInput inAttribute;
//  Cube *cube = flipLines.SetInputCube(ui.GetFileName("TO"), inAttribute);
//  cube->reopen("rw");
//  flipLines.Progress()->SetText("Flipping top to bottom");
//  flipLines.ProcessCubeInPlace(flip, false);
}


void flip(Buffer &in) {
  for(int i = 0; i < in.size() / 2; i++) {
    swap(in[i], in[in.size() - i - 1]);
  }
}

