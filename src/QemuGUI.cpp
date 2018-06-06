#include "QemuGUI.h"


QemuGUI::QemuGUI(QWidget *parent)
    : QMainWindow(parent)
{
    if (QemuGUI::objectName().isEmpty())
        QemuGUI::setObjectName(QStringLiteral("QemuGUI"));
    setWindowTitle(QApplication::translate("QemuGUIClass", "QemuGUI", Q_NULLPTR));
    resize(600, 400);
    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QStringLiteral("menuBar"));
    setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    addToolBar(mainToolBar);
    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));
    setCentralWidget(centralWidget);
    statusBar = new QStatusBar(this);
    statusBar->setObjectName(QStringLiteral("statusBar"));
    setStatusBar(statusBar);

    global_config = new GlobalConfig(this);
    vm_state = VMState::None;

    //main menu
   menuBar->addMenu("What");
    QMenu *settings_menu = new QMenu("Settings", this);
    menuBar->addMenu(settings_menu);
    menuBar->addMenu("do");
    menuBar->addMenu("you");
    menuBar->addMenu("want?");
    
    settings_menu->addAction("Set Qemu", this, SLOT(set_qemu_install_dir()));


    // tool menu
    qemu_install_dir_combo = new QComboBox(this);
    qemu_install_dir_combo->setMinimumWidth(150);

    qemu_play = new QAction(QIcon(":Resources/play.png"), "Play VM", this);
    mainToolBar->addAction(qemu_play);
    connect(qemu_play, SIGNAL(triggered()), this, SLOT(play_machine()));

    qemu_pause = new QAction(QIcon(":Resources/pause_disable.png"), "Pause VM", this);
    mainToolBar->addAction(qemu_pause);
    connect(qemu_pause, SIGNAL(triggered()), this, SLOT(pause_machine()));
    qemu_pause->setEnabled(false);

    qemu_stop = new QAction(QIcon(":Resources/stop_disable.png"), "Stop VM", this);
    mainToolBar->addAction(qemu_stop);
    connect(qemu_stop, SIGNAL(triggered()), this, SLOT(stop_machine()));
    qemu_stop->setEnabled(false);
    
    //mainToolBar->addAction(QIcon(":Resources/play.png"), "Play VM", this, SLOT(play_machine()));
    //mainToolBar->addAction(QIcon(":Resources/pause.png"), "Pause VM", this, SLOT(pause_machine()));
    //mainToolBar->addAction(QIcon(":Resources/stop.png"), "Stop VM", this, SLOT(stop_machine()));
    mainToolBar->addWidget(qemu_install_dir_combo);
    mainToolBar->addSeparator();
    mainToolBar->addAction("Create machine", this, SLOT(create_machine()));
    mainToolBar->addAction("Add existing machine", this, SLOT(add_machine()));
    
    // tab widget	
    tab = new QTabWidget(centralWidget);
    tab->setMinimumWidth(400);
    tab_info = new QWidget(centralWidget);
    tab->addTab(tab_info, "Information about VM");
    rec_replay_tab = new RecordReplayTab();
    tab->addTab(rec_replay_tab, "Record/Replay");
    tab_terminal = new QWidget(centralWidget);
    tab->addTab(tab_terminal, "Terminal");

    // info tab
    propBox = new QGroupBox(tab_info);
    edit_btn = new QPushButton("Edit VM", tab_info);
    info_lbl = new QLabel("", propBox);

    propBox->setMinimumWidth(300);
    propBox->setVisible(false);
    edit_btn->setVisible(false);
    edit_btn->setAutoDefault(true);

    // terminal tab
    terminal_text = new QTextEdit();
    QLineEdit *terminal_cmd = new QLineEdit();
    QPushButton *terminal_send_btn = new QPushButton(">>");
    terminal_text->setReadOnly(true);
    terminal_text->setAutoFillBackground(true);
    QPalette terminal_palette = terminal_text->palette();
    terminal_palette.setColor(QPalette::Base, Qt::black);
    terminal_text->setPalette(terminal_palette);
    terminal_text->setTextColor(Qt::GlobalColor::green);
    terminal_text->setFontFamily("Courier new");
    terminal_send_btn->setFixedWidth(30);
    terminal_send_btn->setAutoDefault(true);
    
    //terminal_palette = terminal_cmd->palette();
    //terminal_palette.setColor(QPalette::Base, Qt::black);
    //terminal_cmd->setPalette(terminal_palette);
    

    QHBoxLayout *cmd_lay = new QHBoxLayout();
    cmd_lay->addWidget(terminal_cmd);
    cmd_lay->addWidget(terminal_send_btn);
    QVBoxLayout *terminal_lay = new QVBoxLayout();
    terminal_lay->addWidget(terminal_text);
    terminal_lay->addLayout(cmd_lay);
    tab_terminal->setLayout(terminal_lay);


    listVM = new QListWidget();
    listVM->setMaximumWidth(300);
    listVM->setUniformItemSizes(true);
    QFont listVMfont;
    listVMfont.setPointSize(10);
    listVM->setFont(listVMfont);

    delete_act = new QAction("Delete VM", listVM);
    exclude_act = new QAction("Exclude VM", listVM);
    listVM->addAction(exclude_act);
    listVM->addAction(delete_act);
    listVM->setContextMenuPolicy(Qt::ActionsContextMenu);

    create_qemu_install_dir_dialog();
    connect_signals();
    fill_listVM_from_config();
    fill_qemu_install_dir_from_config();

    widget_placement();
}

QemuGUI::~QemuGUI()
{
    global_config->set_current_qemu_dir(qemu_install_dir_combo->currentText());
}


void QemuGUI::fill_listVM_from_config()
{
    QList<VMConfig *> exist_vm = global_config->get_exist_vm();

    foreach(VMConfig *vm, exist_vm)
    {
        listVM->addItem(vm->get_name());
        listVM->item(listVM->count() - 1)->setSizeHint(QSize(0, 20));
    }
    listVM->setCurrentRow(0);
    listVM->setFocus();
}

void QemuGUI::widget_placement()
{
    QVBoxLayout *infoGroupLay = new QVBoxLayout(propBox);
    infoGroupLay->addWidget(info_lbl);
    infoGroupLay->addStretch(100);

    QVBoxLayout *infoLay = new QVBoxLayout(tab_info);
    infoLay->addWidget(propBox);
    infoLay->addWidget(edit_btn);

    QHBoxLayout *one = new QHBoxLayout(centralWidget);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->addWidget(listVM);

    QVBoxLayout *lay2 = new QVBoxLayout();
    lay2->addWidget(tab);

    one->addLayout(lay);
    one->addLayout(lay2);
}

void QemuGUI::fill_qemu_install_dir_from_config()
{
    qemu_install_dir_combo->clear();
    qemu_install_dir_list->clear();
    qemu_install_dir_combo->addItem("Add qemu...");
    QStringList qemu_list = global_config->get_qemu_installation_dirs();
    qemu_install_dir_list->addItems(qemu_list);
    for (int i = 0; i < qemu_list.count(); i++)
    {
        qemu_install_dir_combo->insertItem(qemu_install_dir_combo->count() - 1, qemu_list.at(i));
    }
    if (global_config->get_current_qemu_dir() != "")
        qemu_install_dir_combo->setCurrentText(global_config->get_current_qemu_dir());
}

void QemuGUI::create_qemu_install_dir_dialog()
{
    qemu_install_dir_settings = new QDialog(this);
    qemu_install_dir_settings->setWindowTitle("Qemu installation folders");
    qemu_install_dir_list = new QListWidget();
    QPushButton *add_install_dir_btn = new QPushButton("Add QEMU");
    add_install_dir_btn->setAutoDefault(true);
    QPushButton *del_install_dir_btn = new QPushButton("Delete QEMU");
    del_install_dir_btn->setAutoDefault(true);

    QHBoxLayout *buttons_lay = new QHBoxLayout();
    buttons_lay->addWidget(add_install_dir_btn);
    buttons_lay->addWidget(del_install_dir_btn);

    QVBoxLayout *layout = new QVBoxLayout();
    
    layout->addLayout(buttons_lay);
    layout->addWidget(qemu_install_dir_list);

    qemu_install_dir_settings->setLayout(layout);

    connect(add_install_dir_btn, SIGNAL(clicked()), this, SLOT(add_qemu_install_dir_btn()));
    connect(del_install_dir_btn, SIGNAL(clicked()), this, SLOT(del_qemu_install_dir_btn()));
}

void QemuGUI::connect_signals()
{
    /* edit machine */
    connect(edit_btn, SIGNAL(clicked()), this, SLOT(edit_settings()));
    /* list of machines */
    connect(listVM, SIGNAL(itemSelectionChanged()), this, SLOT(listVM_item_selection_changed()));
    connect(listVM, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), 
        this, SLOT(listVM_current_item_changed(QListWidgetItem *, QListWidgetItem *)));
    /* delete VM */
    connect(delete_act, SIGNAL(triggered()), this, SLOT(delete_vm_ctxmenu()));
    /* exclude VM */
    connect(exclude_act, SIGNAL(triggered()), this, SLOT(exclude_vm_ctxmenu()));
    /* create VM */
    connect(global_config, SIGNAL(globalConfig_new_vm_is_complete()), this, SLOT(refresh()));
    
    connect(qemu_install_dir_combo, SIGNAL(activated(int)), this, SLOT(qemu_install_dir_combo_activated(int)));
    connect(qemu_install_dir_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(qemu_install_dir_combo_index_changed(int)));
}

QString QemuGUI::delete_exclude_vm(bool delete_vm)
{
    QString del_vm_name = listVM->currentItem()->text();
    global_config->delete_exclude_vm(del_vm_name, delete_vm);
    // may be return value if all ok, exclude from list

    delete listVM->currentItem();

    if (listVM->count() != 0)
    {
        listVM->setCurrentRow(0);
    }
    return del_vm_name;
}

void QemuGUI::delete_vm_ctxmenu()
{
    int answer = QMessageBox::question(this, "Deleting", "Are you sure?", QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
        QString del_vm_name = delete_exclude_vm(true);
        QMessageBox::information(this, "Success", "Virtual machine " + del_vm_name + " was deleted");
    }
}

void QemuGUI::exclude_vm_ctxmenu()
{
    int answer = QMessageBox::question(this, "Excluding", "Are you sure?", QMessageBox::Yes, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
        QString del_vm_name = delete_exclude_vm(false);
        QMessageBox::information(this, "Success", "Virtual machine " + del_vm_name + " was excluded");
    }
}

void QemuGUI::play_machine()
{
    if (listVM->currentItem())
    {
        if (vm_state == VMState::None && qemu_install_dir_combo->currentIndex() != qemu_install_dir_combo->count() - 1)
        {
            vm_state = VMState::Running;
            terminal_text->clear();
            qemu_play->setIcon(QIcon(":Resources/play_disable.png"));
            qemu_play->setDisabled(true);
            qemu_stop->setIcon(QIcon(":Resources/stop.png"));
            qemu_stop->setEnabled(true);
            qemu_pause->setIcon(QIcon(":Resources/pause.png"));
            qemu_pause->setEnabled(true);

            QThread *thread = new QThread();
            launch_qemu = new QemuLauncher(qemu_install_dir_combo->currentText(),
                global_config->get_vm_by_name(listVM->currentItem()->text()));
            launch_qemu->moveToThread(thread);
            connect(thread, SIGNAL(started()), launch_qemu, SLOT(start_qemu()));
            connect(launch_qemu, SIGNAL(qemu_laucher_finished()), this, SLOT(finish_qemu()));
            thread->start();    
            
            connect(&monitorSocket, SIGNAL(readyRead()), this, SLOT(read_terminal()));
            monitorSocket.connectToHost("127.0.0.1", 23);
            monitorSocket.write("{ \"execute\": \"qmp_capabilities\" }");
        }
        else if (vm_state == VMState::Stopped)
        {
            vm_state = VMState::Running;
            qemu_play->setIcon(QIcon(":Resources/play_disable.png"));
            qemu_play->setDisabled(true);
            qemu_pause->setIcon(QIcon(":Resources/pause.png"));
            qemu_pause->setEnabled(true);
            monitorSocket.write("{ \"execute\": \"cont\" }");
        }
    }
}

void QemuGUI::finish_qemu()
{
    vm_state = VMState::None;
    qemu_play->setIcon(QIcon(":Resources/play.png"));
    qemu_play->setEnabled(true);
    qemu_stop->setIcon(QIcon(":Resources/stop_disable.png"));
    qemu_stop->setEnabled(false);
    qemu_pause->setIcon(QIcon(":Resources/pause_disable.png"));
    qemu_pause->setEnabled(false);
    delete launch_qemu;
    launch_qemu = NULL;
}

void QemuGUI::pause_machine()
{
    vm_state = VMState::Stopped;
    qemu_play->setIcon(QIcon(":Resources/play.png"));
    qemu_play->setEnabled(true);
    qemu_pause->setIcon(QIcon(":Resources/pause_disable.png"));
    qemu_pause->setEnabled(false);
    monitorSocket.write("{ \"execute\": \"stop\" }");
    //terminal_text->insertPlainText("Qemu has stopped\n");
}

void QemuGUI::stop_machine()
{
    launch_qemu->kill_qemu_process();
}

void QemuGUI::create_machine()
{
    createVMWindow = new CreateVMForm(global_config->get_home_dir());
    connect(createVMWindow, SIGNAL(createVM_new_vm_is_complete(VMConfig *)), global_config, SLOT(vm_is_created(VMConfig *)));
}

void QemuGUI::add_machine()
{
    QString filename = QFileDialog::getOpenFileName(this, "Add exist VM", global_config->get_home_dir(), "*.xml");
    if (filename != "")
    {
        if (global_config->add_exist_vm(filename))
            refresh();
        else
            QMessageBox::critical(this, "Error", "Virtual machine cannot be add");
    }
}

void QemuGUI::edit_settings()
{
    settingsWindow = new VMSettingsForm();
}

void QemuGUI::listVM_item_selection_changed()
{
    if (listVM->currentItem())
    {
        VMConfig *vm = global_config->get_vm_by_name(listVM->currentItem()->text());
        if (vm)
            info_lbl->setText(vm->get_vm_info());
        propBox->setVisible(true);
        edit_btn->setVisible(true);
        delete_act->setDisabled(false);
        exclude_act->setDisabled(false);
    }
    else
    {
        propBox->setVisible(false);
        edit_btn->setVisible(false);
        delete_act->setDisabled(true);
        exclude_act->setDisabled(true);
    }
}

void QemuGUI::listVM_current_item_changed(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current)
    {
        QFont font = listVM->font();

        if (previous)
        {
            previous->setTextColor(Qt::GlobalColor::black);
            previous->setFont(font);
        }

        font.setBold(true);
        current->setTextColor(Qt::GlobalColor::darkRed);
        current->setFont(font);
    }
}

void QemuGUI::add_qemu_install_dir_btn()
{
    QString qemu_install_dir = QFileDialog::getExistingDirectory(this, "Select Qemu directory", "");
    if (qemu_install_dir != "")
    {
        if (qemu_install_dir_list->findItems(qemu_install_dir, Qt::MatchFlag::MatchFixedString).size() != 0)
        {
            QMessageBox::critical(this, "Error", qemu_install_dir + " is already added");
            return;
        }

        global_config->add_qemu_installation_dir(qemu_install_dir);
        global_config->set_current_qemu_dir(qemu_install_dir);
        fill_qemu_install_dir_from_config();
    }
}

void QemuGUI::del_qemu_install_dir_btn()
{
    if (qemu_install_dir_list->currentItem())
    {
        int answer = QMessageBox::question(this, "Deleting", "Are you sure?", QMessageBox::Yes, QMessageBox::No);
        if (answer == QMessageBox::Yes)
        {
            global_config->del_qemu_installation_dir(qemu_install_dir_list->currentItem()->text());
            delete qemu_install_dir_list->currentItem();
            fill_qemu_install_dir_from_config();
        }
    }
}

void QemuGUI::refresh()
{
    listVM->clear();
    fill_listVM_from_config();
    listVM->setCurrentItem(listVM->item(listVM->count() - 1));
}

void QemuGUI::set_qemu_install_dir()
{
    qemu_install_dir_settings->show();
}


void QemuGUI::qemu_install_dir_combo_activated(int index)
{
    if (index == qemu_install_dir_combo->count() - 1)
    {
        qemu_install_dir_settings->show();
    }
}


void QemuGUI::qemu_install_dir_combo_index_changed(int index)
{

}

void QemuGUI::read_terminal()
{
    terminal_text->insertPlainText(monitorSocket.readAll());
}


