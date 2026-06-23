#ifndef MyVersion
  #define MyVersion "TEST"
#endif

[Setup]
AppName=SteelToolsMaya
AppVersion={#MyVersion}
LicenseFile=../../../LICENSE.txt
ArchitecturesInstallIn64BitMode=x64
DefaultDirName={autopf}\SteelToolsMaya
DisableDirPage=no
OutputBaseFilename=steelTools_{#MyVersion}

[Files]
Source: "..\..\..\..\releases\maya\*"; DestDir: "{app}"; Flags: recursesubdirs; Excludes: "__pycache__\*";

[Components]
Name: "maya2023"; Description: "Steel Tools for Maya 2023"; Types: custom
Name: "maya2024"; Description: "Steel Tools for Maya 2024"; Types: custom
Name: "maya2025"; Description: "Steel Tools for Maya 2025"; Types: custom

[Code]
var
  PATH, install_dir, module_file_text: string;

procedure InitializeWizard();
begin
  WizardForm.ComponentsList.Checked[0] := False;
  WizardForm.ComponentsList.ItemEnabled[0] := False;
  PATH := '';
  RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2023\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
  if DirExists(PATH) then begin
    WizardForm.ComponentsList.Checked[0] := True;
    WizardForm.ComponentsList.ItemEnabled[0] := True;
  end;

  WizardForm.ComponentsList.Checked[1] := False;
  WizardForm.ComponentsList.ItemEnabled[1] := False;
  PATH := '';
  RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2024\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
  if DirExists(PATH) then begin
    WizardForm.ComponentsList.Checked[1] := True;
    WizardForm.ComponentsList.ItemEnabled[1] := True;
  end;

  WizardForm.ComponentsList.Checked[2] := False;
  WizardForm.ComponentsList.ItemEnabled[2] := False;
  PATH := '';
  RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2025\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
  if DirExists(PATH) then begin
    WizardForm.ComponentsList.Checked[2] := True;
    WizardForm.ComponentsList.ItemEnabled[2] := True;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then begin
    install_dir := ExpandConstant('{app}');

    if WizardIsComponentSelected('maya2023') then begin
      module_file_text := '+ MAYAVERSION:2023 PLATFORM:win64 SteelToolsMaya any ' + install_dir + #13#10 + 'plug-ins: plug-ins/2023';
      RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2023\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
      StringChangeEx(module_file_text, '\', '/', True);
      SaveStringToFile(PATH + '/modules/SteelToolsMaya.mod', module_file_text, False);
    end;

    if WizardIsComponentSelected('maya2024') then begin
      module_file_text := '+ MAYAVERSION:2024 PLATFORM:win64 SteelToolsMaya any ' + install_dir + #13#10 + 'plug-ins: plug-ins/2024';
      RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2024\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
      StringChangeEx(module_file_text, '\', '/', True);
      SaveStringToFile(PATH + '/modules/SteelToolsMaya.mod', module_file_text, False);
    end;

    if WizardIsComponentSelected('maya2025') then begin
      module_file_text := '+ MAYAVERSION:2025 PLATFORM:win64 SteelToolsMaya any ' + install_dir + #13#10 + 'plug-ins: plug-ins/2025';
      RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\Maya\2025\Setup\InstallPath', 'MAYA_INSTALL_LOCATION', PATH);
      StringChangeEx(module_file_text, '\', '/', True);
      SaveStringToFile(PATH + '/modules/SteelToolsMaya.mod', module_file_text, False);
    end;
  end;
end;