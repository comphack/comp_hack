// Emacs mode hint: -*- mode: JavaScript -*-
// https://stackoverflow.com/questions/58311016/qt-installer-scripting-api-cant-select-latest-qt-version-in-online-installer
// https://stackoverflow.com/questions/25105269/silent-install-qt-run-installer-on-ubuntu-server
// https://github.com/wireshark/wireshark/blob/master/tools/qt-installer-windows.qs

// Look for Name elements in
// https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5123/Updates.xml
// Unfortunately it is not possible to disable deps like qt.tools.qtcreator

//[xml]$xml = Get-Content "Updates.xml"
//foreach( $packageUpdate in $xml.Updates.PackageUpdate)
//{
//    Write-Host "`"$($packageUpdate.Name)`","
//}

// 5.13.1 - https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5131/Updates.xml
//   MSVC 2017 32-bit
//   MSVC 2017 64-bit
//   MinGW 7.3.0 32-bit
//   MinGW 7.3.0 64-bit
//   Qt Charts, Qt Data Visualization, Qt Lottie Animation, Qt Purchasing, Qt Virtual Keyboard, Qt WebEngine, Qt Network Authorization, Qt WebGL Streaming Plugin, Qt Script, Qt Debug Information Files
// 5.12.5 - https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5125/Updates.xml
//   MSVC 2017 32-bit
//   MSVC 2017 64-bit
//   MinGW 7.3.0 32-bit
//   MinGW 7.3.0 64-bit
//   Qt Charts, Qt Data Visualization, Qt Purchasing, Qt Virtual Keyboard, Qt WebEngine, Qt Network Authorization, Qt WebGL Streaming Plugin, Qt Script, Qt Debug Information Files
// 5.9.8 - https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_598/Updates.xml
//   MSVC 2015 32-bit
//   MSVC 2017 64-bit
//   MinGW 5.3.0 32-bit
//   Qt Charts, Qt Data Visualization, Qt Purchasing, Qt Virtual Keyboard, Qt WebEngine, Qt Network Authorization, Qt Remote Objects, Qt Speech, Qt Script
// Tools
//   MinGW 7.3.0 32-bit
//   MinGW 7.3.0 64-bit
//   MinGW 5.3.0 32-bit
//   Qt Installer Framework 2.0
//   Qt Installer Framework 3.1

var INSTALL_COMPONENTS = [
    installer.environmentVariable("PLATFORM") == "win64" ?
    "qt.qt5.5123.win64_msvc2017_64" :
    "qt.qt5.5123.win32_msvc2017",
    "qt.qt5.5123.qtwebengine",
];

function Controller() {
    installer.autoRejectMessageBoxes();
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}

Controller.prototype.WelcomePageCallback = function() {
    // click delay here because the next button is initially disabled for ~1 second
    gui.clickButton(buttons.NextButton, 3000);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.DynamicTelemetryPluginFormCallback = function() {
    gui.currentPageWidget().TelemetryPluginForm.statisticGroupBox.disableStatisticRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    // Keep default at "C:\Qt".
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {

    // https://doc-snapshots.qt.io/qtifw-3.1/noninteractive.html
    var page = gui.pageWidgetByObjectName("ComponentSelectionPage");

    var archiveCheckBox = gui.findChild(page, "Archive");
    var latestCheckBox = gui.findChild(page, "Latest releases");
    var fetchButton = gui.findChild(page, "FetchCategoryButton");

    archiveCheckBox.click();
    latestCheckBox.click();
    fetchButton.click();

    var widget = gui.currentPageWidget();

    widget.deselectAll();

    for (var i = 0; i < INSTALL_COMPONENTS.length; i++) {
        widget.selectComponent(INSTALL_COMPONENTS[i]);
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm;
if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
    checkBoxForm.launchQtCreatorCheckBox.checked = false;
}
    gui.clickButton(buttons.FinishButton);
}
