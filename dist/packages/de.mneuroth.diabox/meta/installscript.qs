function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/dia_box.exe", "@StartMenuDir@/DiaBox.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/DiaBox.ico",
            "description=DiaBox");
        component.addOperation("CreateShortcut", "@TargetDir@/dia_box.exe", "@DesktopDir@/DiaBox.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/DiaBox.ico");
    }
}