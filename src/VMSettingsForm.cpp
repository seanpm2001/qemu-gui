#include "VMSettingsForm.h"
#include "DeviceFactory.h"

VMSettingsForm::VMSettingsForm(VMConfig *vmconf, QWidget *parent)
    : QWidget(parent), vm(vmconf)
{
    if (VMSettingsForm::objectName().isEmpty())
        VMSettingsForm::setObjectName(QStringLiteral("VMSettingsForm"));
    resize(420, 340);
    setWindowTitle(QApplication::translate("VMSettingsForm", "VM Settings", Q_NULLPTR));
    setWindowModality(Qt::WindowModality::ApplicationModal);

    deviceTree = new QTreeWidget();
    savecancel_btn = new QDialogButtonBox(QDialogButtonBox::Apply
        | QDialogButtonBox::Save | QDialogButtonBox::Cancel);

    addCmdLineParamsEdit = new QLineEdit();
    addCmdLineParamsEdit->setText(vm->getCmdLine());

    savecancel_btn->button(QDialogButtonBox::Save)->setDefault(true);
    savecancel_btn->button(QDialogButtonBox::Cancel)->setAutoDefault(true);
    savecancel_btn->button(QDialogButtonBox::Apply)->setAutoDefault(true);

    deviceTree->setContextMenuPolicy(Qt::CustomContextMenu);
    deviceTree->setHeaderHidden(1);
    deviceTree->setColumnCount(1);

    addDev = NULL;
    addedDevices.clear();

    foreach(Device *dev, vm->getSystemDevice()->getDevices())
    {
        DeviceTreeItem *it = new DeviceTreeItem(dev);
        if (dev->isDeviceInvisible())
        {
            delete it;
        }
        else
        {
            deviceTree->invisibleRootItem()->addChild(it);
        }
    }

    connect_signals();
    widget_placement();
    show();
}

VMSettingsForm::~VMSettingsForm()
{
}

void VMSettingsForm::connect_signals()
{
    connect(deviceTree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
        SLOT(onDeviceTreeItemClicked(QTreeWidgetItem *, int)));

    connect(savecancel_btn, &QDialogButtonBox::accepted,
        this, &VMSettingsForm::save_settings);
    connect(savecancel_btn, &QDialogButtonBox::rejected,
        this, &VMSettingsForm::closeForm);
    connect(savecancel_btn->button(QDialogButtonBox::Apply), &QPushButton::clicked,
        this, &VMSettingsForm::applySettings);

    connect(deviceTree, &QTreeWidget::customContextMenuRequested,
        this, &VMSettingsForm::showContextMenu);

    connect(&menu, &QMenu::aboutToHide, this, &VMSettingsForm::menuClose);
}

QWidget* VMSettingsForm::emptyForm()
{
    QGroupBox *deviceInfoGroup = new QGroupBox();
    QHBoxLayout *infoLay = new QHBoxLayout();
    QLabel *infoLbl = new QLabel("<No information>", deviceInfoGroup);
    infoLay->addStretch(50);
    infoLay->addWidget(infoLbl);
    infoLay->addStretch(50);
    deviceInfoGroup->setLayout(infoLay);
    return deviceInfoGroup;
}

void VMSettingsForm::closeEvent(QCloseEvent *event)
{
    closeForm();
}

Device * VMSettingsForm::isDevicesValid(Device *device)
{
    Device *retDevice = NULL;
    foreach(Device *dev, device->getDevices())
    {
        if (!dev->isDeviceValid())
        {
            retDevice = dev;
        } 
        if (!retDevice)
        {
            retDevice = isDevicesValid(dev);
        }
    }
    return retDevice;
}

void VMSettingsForm::widget_placement()
{
    splitter = new QSplitter();
    splitter->addWidget(deviceTree);
    splitter->addWidget(emptyForm());

    splitter->setSizes(QList<int>{150, 250});

    QGroupBox *addCmdLineParamsGrpoup = new QGroupBox("Additional command line parameters");
    QVBoxLayout *addCmdLay = new QVBoxLayout();
    addCmdLay->addWidget(addCmdLineParamsEdit);
    addCmdLineParamsGrpoup->setLayout(addCmdLay);


    QVBoxLayout *main = new QVBoxLayout(this);
    main->addWidget(splitter);
    main->addWidget(addCmdLineParamsGrpoup);
    main->addStretch(100);
    main->addWidget(savecancel_btn);
}

void VMSettingsForm::applySettings()
{
    int answer = QMessageBox::question(this, "Saving",
        "All recorded executions will be removed. Are you sure?",
        QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
        vm->setCmdLine(addCmdLineParamsEdit->text());
        vm->save_vm_config();
        vm->remove_directory_vm(vm->getPathRRDir());
        emit settingsDeleteRecords();
    }
}

void VMSettingsForm::save_settings()
{
    Device *dev = isDevicesValid(vm->getSystemDevice());
    if (!dev)
    {
        applySettings();
        close();
    }
    else
    {
        QMessageBox::critical(this, "Error",
            "Device " + dev->getDescription() + " has invalid information");
    }
}

void VMSettingsForm::onDeviceTreeItemClicked(QTreeWidgetItem *item, int column)
{
    DeviceTreeItem *devItem = dynamic_cast<DeviceTreeItem*>(item);
    Q_ASSERT(devItem);

    Device *dev = devItem->getDevice();
    QWidget *form = dev->getEditorForm();

    delete splitter->widget(1);
    if (form)
    {
        splitter->addWidget(form);
    }
    else
    {
        splitter->addWidget(emptyForm());
    }
    splitter->setSizes(QList<int>{150, 250});
}

void VMSettingsForm::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = deviceTree->itemAt(pos);
    if (item)
    {
        deviceTree->setCurrentItem(item);

        DeviceTreeItem *devItem = dynamic_cast<DeviceTreeItem*>(item);
        Q_ASSERT(devItem);
        Device *dev = devItem->getDevice();

        addDev = new AddDeviceForm(dev);
        addDev->setAttribute(Qt::WA_DeleteOnClose);
        if (addDev->getAddDevicesCount())
        {
            QAction *addDeviceAct = new QAction("Add device", this);
            addDeviceAct->setStatusTip(tr("Add device"));

            connect(addDeviceAct, SIGNAL(triggered()), addDev, SLOT(addDevice()));
            connect(addDev, SIGNAL(deviceWantsToAdd(Device *)),
                this, SLOT(addNewDevice(Device *)));

            menu.addAction(addDeviceAct);
        }

        if (dev->isRemovable())
        {
            QAction *removeDeviceAct = new QAction("Remove device", this);
            removeDeviceAct->setStatusTip(tr("Remove device"));
            connect(removeDeviceAct, SIGNAL(triggered()), this, SLOT(removeDevice()));
            menu.addAction(removeDeviceAct);
        }
        if (!menu.isEmpty())
        {
            menu.exec(deviceTree->mapToGlobal(pos));
        }
        else
        {
            delete addDev;
        }
    }
    /*
    TODO
    else
    {
        qDebug() << "add add add";
        QAction *addDeviceAct = new QAction("Add system device (dont use it)", this);
        addDeviceAct->setStatusTip(tr("Add system device"));

        AddDeviceForm *addSystemDev = NULL;//new AddDeviceForm(QStringList({ "Usb", "...", }));
        addSystemDev->setAttribute(Qt::WA_DeleteOnClose);

        connect(addDeviceAct, SIGNAL(triggered()), addSystemDev, SLOT(addDevice()));
        connect(addSystemDev, SIGNAL(deviceWantsToAdd(const QString &)),
            this, SLOT(addNewSystemDevice(const QString &)));

        QMenu menu(this);
        menu.addAction(addDeviceAct);

        menu.exec(deviceTree->mapToGlobal(pos));
    }
    */
}

void VMSettingsForm::addNewDevice(Device *newDevice)
{
    /* TODO: devices can have various count of children */
    DeviceTreeItem *devItem = dynamic_cast<DeviceTreeItem*>(deviceTree->currentItem());
    Q_ASSERT(devItem);
    Device *dev = devItem->getDevice();
    if (devItem->childCount() < dev->getMaxCountDevices())
    {
        addedDevices.append(newDevice);
        dev->addDevice(newDevice);
        DeviceTreeItem *it = new DeviceTreeItem(newDevice);
        deviceTree->currentItem()->addChild(it);
        deviceTree->setCurrentItem(it);
        onDeviceTreeItemClicked(it, 0);
    }
    else
    {
        QMessageBox::critical(this, "Error", "Too much devices");
    }
}

void VMSettingsForm::addNewSystemDevice(const QString &devName)
{
    if (QString::compare(devName, "Usb") == 0)
    {
        vm->addUsbDevice();
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, "Usb");
        deviceTree->invisibleRootItem()->addChild(item);
    }
}

void VMSettingsForm::removeDevice()
{
    DeviceTreeItem *devItem = dynamic_cast<DeviceTreeItem *>(deviceTree->currentItem());
    Q_ASSERT(devItem);
    Device *dev = devItem->getDevice();
    int answer = QMessageBox::question(this, "Removing",
        "Device " + dev->getDescription() + " will be removed. Are you sure?",
        QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
        Device *devParent = dynamic_cast<Device *>(dev->parent());
        Q_ASSERT(devParent);
        devParent->removeDevice(dev);
        deviceTree->setCurrentItem(devItem->parent());
        onDeviceTreeItemClicked(devItem->parent(), 0);
        delete devItem;        
    }
}

void VMSettingsForm::menuClose()
{
    if (!menu.activeAction())
    {
        delete addDev;
    }
    menu.clear();
}

void VMSettingsForm::closeForm()
{
    foreach(Device *dev, addedDevices)
    {
        Device *devParent = dynamic_cast<Device *>(dev->parent());
        Q_ASSERT(devParent);
        devParent->removeDevice(dev);
    }
    addedDevices.clear();
    vm->readVMConfig();
    close();
}

/***************************************************************************
 * DeviceTreeItem                                                          *
 ***************************************************************************/

DeviceTreeItem::DeviceTreeItem(Device *dev)
{
    initDevice(dev);
}

void DeviceTreeItem::initDevice(Device *dev)
{
    device = dev;
    setText(0, device->getDescription());
    foreach(Device *dev, device->getDevices())
    {
        DeviceTreeItem *it = new DeviceTreeItem(dev);
        addChild(it);
    }
}

Device *DeviceTreeItem::getDevice()
{
    return device;
}

