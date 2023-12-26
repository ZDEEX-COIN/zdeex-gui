// Copyright 2019-2022 The Hush Developers
// Released under the GPLv3
#include "mainwindow.h"
#include "addressbook.h"
#include "viewalladdresses.h"
#include "validateaddress.h"
#include "ui_mainwindow.h"
#include "ui_addressbook.h"
#include "ui_privkey.h"
#include "ui_qrcode.h"
#include "ui_viewkey.h"
#include "ui_about.h"
#include "ui_settings.h"
#include "ui_viewalladdresses.h"
#include "ui_validateaddress.h"
#include "ui_rescandialog.h"
#include "ui_getblock.h"
#include "rpc.h"
#include "balancestablemodel.h"
#include "settings.h"
#include "version.h"
#include "senttxstore.h"
#include "connection.h"
#include "requestdialog.h"
#include <QLCDNumber>
#include <QThread>
#include "sd.h"

extern bool iszdeex;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Include css
    QString theme_name;
    try
    {
       theme_name = Settings::getInstance()->get_theme_name();
    } catch (...)
    {
        qDebug() << __func__ << ": exception!";

        if(iszdeex){
            theme_name = "zdeex";
        }else{
            theme_name = "dark";
        }
    }

    this->slot_change_theme(theme_name);

    ui->setupUi(this);
    logger = new Logger(this, QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(iszdeex ? "SilentZdeeX.log" : "SilentDragon.log"));

    // Status Bar
    setupStatusBar();

    // Settings editor
    setupSettingsModal();

    // Set up actions
    QObject::connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    QObject::connect(ui->actionTelegram, &QAction::triggered, this, &MainWindow::telegram);
    QObject::connect(ui->actionReportBug, &QAction::triggered, this, &MainWindow::reportbug);
    QObject::connect(ui->actionWebsite, &QAction::triggered, this, &MainWindow::website);

    // Set up check for updates action
    QObject::connect(ui->actionCheck_for_Updates, &QAction::triggered, [=] () {
        // Silent is false, so show notification even if no update was found
        rpc->checkForUpdate(false);
    });

    // Request hush
    QObject::connect(ui->actionRequest_hush, &QAction::triggered, [=]() {
        RequestDialog::showRequestZcash(this);
    });

    // Pay Hush URI
    QObject::connect(ui->actionPay_URI, &QAction::triggered, [=] () {
        payHushURI();
    });

    // Import Private Key
    QObject::connect(ui->actionImport_Private_Key, &QAction::triggered, this, &MainWindow::importPrivKey);

    // Export All Private Keys
    QObject::connect(ui->actionExport_All_Private_Keys, &QAction::triggered, this, &MainWindow::exportAllKeys);

    // Backup wallet.dat
    QObject::connect(ui->actionBackup_wallet_dat, &QAction::triggered, this, &MainWindow::backupWalletDat);

    // Export transactions
    QObject::connect(ui->actionExport_transactions, &QAction::triggered, this, &MainWindow::exportTransactions);

    // Validate Address
    QObject::connect(ui->actionValidate_Address, &QAction::triggered, this, &MainWindow::validateAddress);

    // Get Block
    QObject::connect(ui->actionGet_Block, &QAction::triggered, this, &MainWindow::getBlock);

    // Address Book
    QObject::connect(ui->action_Address_Book, &QAction::triggered, this, &MainWindow::addressBook);

    // Set up about action
    QObject::connect(ui->actionAbout, &QAction::triggered, [=] () {
        QDialog aboutDialog(this);
        Ui_about about;
        about.setupUi(&aboutDialog);
        Settings::saveRestore(&aboutDialog);

        QString version    = QString("Version ") % QString(APP_VERSION) % " (" % QString(__DATE__) % ") using QT " % qVersion();
        about.versionLabel->setText(version);

        aboutDialog.exec();
    });

    // Initialize to the balances tab
    ui->tabWidget->setCurrentIndex(0);


    setupSendTab();
    setupTransactionsTab();
    setupReceiveTab();
    setupBalancesTab();
    setupMarketTab();
    //setupChatTab();
    setupHushTab();
    setupPeersTab();

    rpc = new RPC(this);
    qDebug() << "Created RPC";

    setupMiningTab();

    restoreSavedStates();
}

void MainWindow::restoreSavedStates() {
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());

    ui->balancesTable->horizontalHeader()->restoreState(s.value("baltablegeometry").toByteArray());
    ui->transactionsTable->horizontalHeader()->restoreState(s.value("tratablegeometry").toByteArray());
}

void MainWindow::doClose() {
    closeEvent(nullptr);
}

void MainWindow::retranslateMiningTab() {
    DEBUG("retranslating mining tab");

    auto tab = ui->tabWidget->template findChild<QWidget *>("Mining");
    if(tab != nullptr) {
        DEBUG("found Mining tab");
        ui->tabWidget->setTabText(ui->tabWidget->indexOf(tab), QObject::tr("Mining"));
    } else {
        DEBUG("no Mining tab found!");
        return;
    }

    auto button1 = tab->template findChild<QPushButton *>("stopmining");
    if(button1 != nullptr) {
        DEBUG("found stop mining button " << button1->objectName());
        button1->setText( QObject::tr("Stop Mining") );
    }

    auto button2 = tab->template findChild<QPushButton *>("startmining");
    if(button2 != nullptr) {
        DEBUG("found start mining button " << button2->objectName());
        button2->setText( QObject::tr("Start Mining") );
        button2->setObjectName("startmining");
    }

    auto label2 = tab->findChild<QLabel *>("mininglabel2");
    if(label2 != nullptr) {
        DEBUG("found mininglabel2");
        label2->setText( QObject::tr("Mining threads") );
    }
    auto label3 = tab->findChild<QLabel *>("mininglabel3");
    if(label3 != nullptr) {
        label3->setText( QObject::tr("Local Hashrate (hashes/sec)") );
    }
    auto label4 = tab->findChild<QLabel *>("mininglabel4");
    if(label4 != nullptr) {
        label4->setText( QObject::tr("Network Hashrate (hashes/sec)") );
    }
    auto label5 = tab->findChild<QLabel *>("mininglabel5");
    if(label5 != nullptr) {
        label5->setText( QObject::tr("Difficulty") );
    }
    auto label6 = tab->findChild<QLabel *>("mininglabel6");
    if(label6 != nullptr) {
        label6->setText( QObject::tr("Estimated Hours To Find A Block") );
    }
    auto label7 = tab->findChild<QLabel *>("mininglabel7");
    if(label7 != nullptr) {
        label7->setText( QObject::tr("Select the number of threads to mine with:") );
    }

    auto combo = tab->findChild<QComboBox *>("genproclimit");
    if(combo != nullptr) {
        DEBUG("found genproclimit combo");

        int hwc = std::thread::hardware_concurrency();
        // TODO: Is there a better way than recreating this combobox?
        combo->clear();
        auto threadStr  = tr("thread");
        auto threadsStr = tr("threads");
        // give options from 1 to hwc/2 , which should represent physical CPUs
        for(int i=0; i < hwc/2; i++) {
            combo->insertItem(i, QString::number(i+1) % " " % (i+1==1 ? threadStr : threadsStr));
        }
    }
}

// Called every time, when a menu entry of the language menu is called
void MainWindow::slotLanguageChanged(QString lang)
{
    qDebug() << __func__ << ": lang=" << lang;
    if(lang != "") {
        // load the language
        loadLanguage(lang);

        QDialog settingsDialog(this);
        qDebug() << __func__ << ": retranslating settingsDialog";
        settings.retranslateUi(&settingsDialog);

        retranslateMiningTab();
    }

}


void switchTranslator(QTranslator& translator, const QString& filename) {
    qDebug() << __func__ << ": filename=" << filename;
    // remove the old translator
    qApp->removeTranslator(&translator);

    // load the new translator
    QString path = QApplication::applicationDirPath();
    if (iszdeex) {
        path.append("/res-zdeex/");
    }
    else{
        path.append("/res/");
    }
    qDebug() << __func__ << ": attempting to load " << path + filename;
    if(translator.load(path + filename)) {
        qApp->installTranslator(&translator);
    } else {
        qDebug() << __func__ << ": translation path does not exist! " << path + filename;
    }
}

void MainWindow::loadLanguage(QString& rLanguage) {
    qDebug() << __func__ << ": currLang=" << m_currLang << "  rLanguage=" << rLanguage;

    QString lang = rLanguage;

    // this allows us to call this function with just a locale such as "zh"
    if(lang.right(1) == ")") {
        lang.chop(1); // remove trailing )
    }

    // remove everything up to and including the first (
    lang = lang.remove(0, lang.indexOf("(") + 1);

    // NOTE: language codes can be 2 or 3 letters
    // https://www.loc.gov/standards/iso639-2/php/code_list.php

    QString languageName;
    if(m_currLang != lang) {
        qDebug() << __func__ << ": changing language to lang=" << lang;
        m_currLang = lang;
        QLocale locale = QLocale(m_currLang);

        qDebug() << __func__ << ": locale nativeLanguage=" << locale.nativeLanguageName();

        // an invalid locale such as "zz" will give the C locale which has no native language name
        if (locale.nativeLanguageName() == "") {
            qDebug() << __func__ << ": detected invalid language in config file, defaulting to en";
            locale = QLocale("en");
            Settings::getInstance()->set_language("en");
            m_currLang = "en";
            lang = "en";
        }
        qDebug() << __func__ << ": locale=" << locale;
        QLocale::setDefault(locale);
        qDebug() << __func__ << ": setDefault locale=" << locale;
        languageName = locale.nativeLanguageName(); //locale.language());
        qDebug() << __func__ << ": languageName=" << languageName;

        switchTranslator(m_translator, QString("silentdragon_%1.qm").arg(lang));
        switchTranslator(m_translatorQt, QString("qt_%1.qm").arg(lang));

        // TODO: this likely wont work for RTL languages like Arabic
        auto first = QString(languageName.at(0)).toUpper();
        languageName = first + languageName.right(languageName.size()-1);
        if( lang == "en" ) {
            languageName.replace("American ","");
        }
        ui->statusBar->showMessage(tr("Language changed to") + " " + languageName + " (" + lang + ")");
    }

    // write this language (the locale shortcode) out to config file
    if (lang != "") {
        // only write valid languages to config file
        Settings::getInstance()->set_language(lang);
    }
}

void MainWindow::changeEvent(QEvent* event) {
 if(0 != event) {
  switch(event->type()) {
   // this event is sent if a translator is loaded
   case QEvent::LanguageChange:
    qDebug() << __func__ << ": QEvent::LanguageChange changeEvent";
    ui->retranslateUi(this);
    break;

   // this event is sent, if the system, language changes
   case QEvent::LocaleChange:
   {
    QString locale = QLocale::system().name();
    locale.truncate(locale.lastIndexOf('_'));
    qDebug() << __func__ << ": QEvent::LocaleChange changeEvent locale=" << locale;
    loadLanguage(locale);
   }
   break;
   default:
    qDebug() << __func__ << ": " << event->type();
  }
 }
 QMainWindow::changeEvent(event);
}


void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings s;

    s.setValue("geometry", saveGeometry());
    s.setValue("baltablegeometry", ui->balancesTable->horizontalHeader()->saveState());
    s.setValue("tratablegeometry", ui->transactionsTable->horizontalHeader()->saveState());

    s.sync();

    // Let the RPC know to shut down any running service.
    rpc->shutdownHushd();

    // Bubble up
    if (event)
        QMainWindow::closeEvent(event);
}

void MainWindow::setupStatusBar() {
    // Status Bar
    loadingLabel = new QLabel();
    loadingMovie = new QMovie(":/icons/loading.gif");
    loadingMovie->setScaledSize(QSize(32, 16));
    loadingMovie->start();
    loadingLabel->setAttribute(Qt::WA_NoSystemBackground);
    loadingLabel->setMovie(loadingMovie);

    ui->statusBar->addPermanentWidget(loadingLabel);
    loadingLabel->setVisible(false);

    // Custom status bar menu
    ui->statusBar->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->statusBar, &QStatusBar::customContextMenuRequested, [=](QPoint pos) {
        auto msg = ui->statusBar->currentMessage();
        QMenu menu(this);

        if (!msg.isEmpty() && msg.startsWith(Settings::txidStatusMessage)) {
            auto txid = msg.split(":")[1].trimmed();
            menu.addAction("Copy txid", [=]() {
                QGuiApplication::clipboard()->setText(txid);
            });
            menu.addAction("Copy block explorer link", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.txExplorerUrl + txid;
                DEBUG("explorer url=" << url);
                QGuiApplication::clipboard()->setText(url);
            });
            menu.addAction("Copy Tor block explorer link", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.onionTxExplorerUrl + txid;
                DEBUG("explorer url=" << url);
                QGuiApplication::clipboard()->setText(url);
            });
            menu.addAction("View tx on block explorer", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.txExplorerUrl + txid;
                DEBUG("explorer url=" << url);
                QDesktopServices::openUrl(QUrl(url));
            });
            menu.addAction("View tx via Tor block explorer", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.onionTxExplorerUrl + txid;
                DEBUG("explorer url=" << url);
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        menu.addAction("Refresh", [=]() {
            rpc->refresh(true);
        });
        QPoint gpos(mapToGlobal(pos).x(), mapToGlobal(pos).y() + this->height() - ui->statusBar->height());
        menu.exec(gpos);
    });

    statusLabel = new QLabel();
    ui->statusBar->addPermanentWidget(statusLabel);

    statusIcon = new QLabel();
    ui->statusBar->addPermanentWidget(statusIcon);
}

void MainWindow::setupSettingsModal() {
    // Set up File -> Settings action
    QObject::connect(ui->actionSettings, &QAction::triggered, [=]() {
        QDialog settingsDialog(this);
        //Ui_Settings settings;
        settings.setupUi(&settingsDialog);
        Settings::saveRestore(&settingsDialog);

        // Setup save sent check box
        QObject::connect(settings.chkSaveTxs, &QCheckBox::stateChanged, [=](auto checked) {
            Settings::getInstance()->setSaveZtxs(checked);
        });

        QString currency_name;
        try {
            currency_name = Settings::getInstance()->get_currency_name();
        } catch (const std::exception& e) {
            qDebug() << QString("Currency name exception! : ");
            currency_name = "BTC";
        }

        this->slot_change_currency(currency_name);

        // Setup clear button
        QObject::connect(settings.btnClearSaved, &QCheckBox::clicked, [=]() {
            if (QMessageBox::warning(this, "Clear saved history?",
                "Shielded z-Address transactions are stored locally in your wallet, outside hushd. You may delete this saved information safely any time for your privacy.\nDo you want to delete the saved shielded transactions now?",
                QMessageBox::Yes, QMessageBox::Cancel)) {
                    SentTxStore::deleteHistory();
                    // Reload after the clear button so existing txs disappear
                    rpc->refresh(true);
            }
        });

        qDebug() << __func__ << ": Setup rescan button";
        QObject::connect(settings.rescanButton, &QPushButton::clicked, [=] () {
            this->rescanButtonClicked(1);
        });

        int theme_index = settings.comboBoxTheme->findText(Settings::getInstance()->get_theme_name(), Qt::MatchExactly);
        settings.comboBoxTheme->setCurrentIndex(theme_index);

        QObject::connect(settings.comboBoxTheme, &QComboBox::currentTextChanged, [=] (QString theme_name) {
            this->slot_change_theme(theme_name);
            // QMessageBox::information(this, tr("Theme Change"), tr("This change can take a few seconds."), QMessageBox::Ok);
            // For some reason, changing language also triggers this
            //ui->statusBar->showMessage(tr("Theme changed to ") + theme_name);
        });


        // Set local currency
        QString ticker = Settings::getInstance()->get_currency_name();
        int currency_index = settings.comboBoxCurrency->findText(ticker, Qt::MatchExactly);
        settings.comboBoxCurrency->setCurrentIndex(currency_index);
        QObject::connect(settings.comboBoxCurrency, &QComboBox::currentTextChanged, [=] (QString ticker) {
            this->slot_change_currency(ticker);
            rpc->refresh(true);
            ui->statusBar->showMessage(tr("Currency changed to") + " " + ticker);
            // QMessageBox::information(this, tr("Currency Change"), tr("This change can take a few seconds."), QMessageBox::Ok);
        });

        // Save sent transactions
        settings.chkSaveTxs->setChecked(Settings::getInstance()->getSaveZtxs());

        // Custom fees
        settings.chkCustomFees->setChecked(Settings::getInstance()->getAllowCustomFees());

        // Auto shielding
        settings.chkAutoShield->setChecked(Settings::getInstance()->getAutoShield());

        // Check for updates
        settings.chkCheckUpdates->setChecked(Settings::getInstance()->getCheckForUpdates());

        // Fetch prices
        settings.chkFetchPrices->setChecked(Settings::getInstance()->getAllowFetchPrices());

        // Use Tor
        bool isUsingTor = false;
        if (rpc->getConnection() != nullptr) {
            isUsingTor = !rpc->getConnection()->config->proxy.isEmpty();
        }
        settings.chkTor->setChecked(isUsingTor);
        if (rpc->getEHushD() == nullptr) {
            settings.chkTor->setEnabled(false);
            settings.lblTor->setEnabled(false);
            QString tooltip = tr("Tor configuration is available only when running an embedded hushd.");
            settings.chkTor->setToolTip(tooltip);
            settings.lblTor->setToolTip(tooltip);
        }

        // Wallet Size
        if (rpc->getConnection() != nullptr) {
            int size = 0;
            qDebug() << __func__ << ": settings hushDir=" << rpc->getConnection()->config->hushDir;

            QDir hushdir(rpc->getConnection()->config->hushDir);
            QFile WalletSize(hushdir.filePath("wallet.dat"));

            if (WalletSize.open(QIODevice::ReadOnly)){
                size = WalletSize.size() / 1000000;  //when file does open.
                //QString size1 = QString::number(size) ;
                settings.WalletSize->setText(QString::number(size));
                WalletSize.close();
            }
        }

        // Use Consolidation
        bool isUsingConsolidation = false;
        if (rpc->getConnection() != nullptr) {
            isUsingConsolidation = !rpc->getConnection()->config->consolidation.isEmpty() == true;
        }
        settings.chkConso->setChecked(isUsingConsolidation);
        if (rpc->getEHushD() == nullptr) {
            settings.chkConso->setEnabled(false);
        }

        // Use Deletetx
        bool isUsingDeletetx = false;
        if (rpc->getConnection() != nullptr) {
            isUsingDeletetx = !rpc->getConnection()->config->deletetx.isEmpty() == true;
        }
        settings.chkDeletetx->setChecked(isUsingDeletetx);
        if (rpc->getEHushD() == nullptr) {
            settings.chkDeletetx->setEnabled(false);
        }

        // Use Zindex
        bool isUsingZindex = false;
        if (rpc->getConnection() != nullptr) {
            isUsingZindex = !rpc->getConnection()->config->zindex.isEmpty() == true;
        }
        settings.chkzindex->setChecked(isUsingZindex);
        if (rpc->getEHushD() == nullptr) {
            settings.chkzindex->setEnabled(false);
        }

        // Connection Settings
        QIntValidator validator(0, 65535);
        settings.port->setValidator(&validator);

        // If values are coming from HUSH3.conf, then disable all the fields
        auto hushConfLocation = Settings::getInstance()->getHushdConfLocation();
        if (!hushConfLocation.isEmpty()) {
            settings.confMsg->setText("Settings are being read from \n" + hushConfLocation);
            settings.hostname->setReadOnly(true);
            settings.port->setReadOnly(true);
            settings.rpcuser->setReadOnly(true);
            settings.rpcpassword->setReadOnly(true);
        } else {
            settings.confMsg->setText("No " % hushConfLocation % " found. Please configure connection manually.");
            settings.hostname->setEnabled(true);
            settings.port->setEnabled(true);
            settings.rpcuser->setEnabled(true);
            settings.rpcpassword->setEnabled(true);
        }

        // Load current values into the dialog
        auto conf = Settings::getInstance()->getSettings();
        settings.hostname->setText(conf.host);
        settings.port->setText(conf.port);
        settings.rpcuser->setText(conf.rpcuser);
        settings.rpcpassword->setText(conf.rpcpassword);

        // Load current explorer values into the dialog
        auto explorer = Settings::getInstance()->getExplorer();
        settings.txExplorerUrl->setText(explorer.txExplorerUrl);
        settings.addressExplorerUrl->setText(explorer.addressExplorerUrl);
        settings.onionTxExplorerUrl->setText(explorer.onionTxExplorerUrl);
        settings.onionAddressExplorerUrl->setText(explorer.onionAddressExplorerUrl);

        // format systems language
        QString defaultLocale = QLocale::system().name(); // e.g. "de_DE"
        defaultLocale.truncate(defaultLocale.lastIndexOf('_')); // e.g. "de"

        // Set the current language to the default system language
        // TODO: this will need to change when we read/write selected language to config on disk
        //m_currLang = defaultLocale;
        //qDebug() << __func__ << ": changed m_currLang to " << defaultLocale;

        m_currLang = Settings::getInstance()->get_language();
        qDebug() << __func__ << ": got a currLang=" << m_currLang << " from config file";

        //QString defaultLang = QLocale::languageToString(QLocale("en").language());
        settings.comboBoxLanguage->addItem("English (en)");

        m_langPath = QApplication::applicationDirPath();
        m_langPath.append("/res");

        qDebug() << __func__ <<": defaultLocale=" << defaultLocale << " m_langPath=" << m_langPath;;

        QDir dir(m_langPath);
        QStringList fileNames = dir.entryList(QStringList("silentdragon_*.qm"));

        qDebug() << __func__ <<": found " << fileNames.size() << " translations";


        // create language drop down dynamically
        for (int i = 0; i < fileNames.size(); ++i) {
            // get locale extracted by filename
            QString locale;
            locale = fileNames[i]; // "silentdragon_de.qm"
            locale.truncate(locale.lastIndexOf('.')); // "silentdragon_de"
            locale.remove(0, locale.lastIndexOf('_') + 1); // "de"

            QString lang = QLocale(locale).nativeLanguageName(); //locale.language());

            // TODO: this likely wont work for RTL languages like Arabic
            // uppercase the first letter of all languages
            auto first = QString(lang.at(0)).toUpper();
            lang = first + lang.right(lang.size()-1);

            //settings.comboBoxLanguage->addItem(action);
            settings.comboBoxLanguage->addItem(lang + " (" + locale + ")");
            qDebug() << __func__ << ": added lang=" << lang << " locale=" << locale << " defaultLocale=" << defaultLocale << " m_currLang=" << m_currLang;
            qDebug() << __func__ << ": m_currLang=" << m_currLang << " ?= locale=" << locale;

            //if (defaultLocale == locale) {
            if (m_currLang == locale) {
                settings.comboBoxLanguage->setCurrentIndex(i+1);
                qDebug() << " set defaultLocale=" << locale << " to checked!!!";
            }
        }

        settings.comboBoxLanguage->model()->sort(0,Qt::AscendingOrder);
        qDebug() << __func__ <<": sorted translations";

        //QString lang = QLocale::languageToString(QLocale(m_currLang).language());
        QString lang = QLocale(m_currLang).nativeLanguageName(); //locale.language());

        auto first = QString(lang.at(0)).toUpper();
        lang = first + lang.right(lang.size()-1);

        if (m_currLang == "en") {
            // we have just 1 English translation
            // en_US will render as "American English", so fix that
            lang.replace("American ","");
        }

        qDebug() << __func__ << ": looking for " << lang + " (" + m_currLang + ")";
        //qDebug() << __func__ << ": looking for " << m_currLang;
        int lang_index = settings.comboBoxLanguage->findText(lang + " (" + m_currLang + ")", Qt::MatchExactly);

        qDebug() << __func__ << ": setting comboBoxLanguage index to " << lang_index;
        settings.comboBoxLanguage->setCurrentIndex(lang_index);

        QObject::connect(settings.comboBoxLanguage, &QComboBox::currentTextChanged, [=] (QString lang) {
            qDebug() << "comboBoxLanguage.currentTextChanged lang=" << lang;
            this->slotLanguageChanged(lang);
            //QMessageBox::information(this, tr("Language Changed"), tr("This change can take a few seconds."), QMessageBox::Ok);
        });

        // Options tab by default
        settings.tabWidget->setCurrentIndex(1);

        // Enable the troubleshooting options only if using embedded hushd
        if (!rpc->isEmbedded()) {
            //settings.chkRescan->setEnabled(false);
            //settings.chkRescan->setToolTip(tr("You're using an external hushd. Please restart hushd with -rescan"));

            settings.chkReindex->setEnabled(false);
            settings.chkReindex->setToolTip(tr("You're using an external hushd. Please restart hushd with -reindex"));
        }

        if (settingsDialog.exec() == QDialog::Accepted) {
            qDebug() << "Setting dialog box accepted";
            // Custom fees
            bool customFees = settings.chkCustomFees->isChecked();
            Settings::getInstance()->setAllowCustomFees(customFees);
            ui->minerFeeAmt->setReadOnly(!customFees);
            if (!customFees)
                ui->minerFeeAmt->setText(Settings::getDecimalString(Settings::getMinerFee()));

            // Auto shield
            Settings::getInstance()->setAutoShield(settings.chkAutoShield->isChecked());

            // Check for updates
            Settings::getInstance()->setCheckForUpdates(settings.chkCheckUpdates->isChecked());

            // Allow fetching prices
            Settings::getInstance()->setAllowFetchPrices(settings.chkFetchPrices->isChecked());

            if (!isUsingTor && settings.chkTor->isChecked()) {
                // If "use tor" was previously unchecked and now checked
                QString torProxy= settings.torProxy->text();
                QString torPort = settings.torPort->text();
                QString proxyConfig = "proxy="%torProxy%":"%torPort;
                Settings::addToHushConf(hushConfLocation, proxyConfig);
                rpc->getConnection()->config->proxy = proxyConfig;

                QMessageBox::information(this, tr("Enable Tor"),
                    tr("Connection over Tor has been enabled. To use this feature, you need to restart SilentDragon."),
                    QMessageBox::Ok);
            }

            if (isUsingTor && !settings.chkTor->isChecked()) {
                // If "use tor" was previously checked and now is unchecked
                Settings::removeFromHushConf(hushConfLocation, "proxy");
                rpc->getConnection()->config->proxy.clear();

                QMessageBox::information(this, tr("Disable Tor"),
                    tr("Connection over Tor has been disabled. To fully disconnect from Tor, you need to restart SilentDragon."),
                    QMessageBox::Ok);
            }

            if (hushConfLocation.isEmpty()) {
                // Save settings
                Settings::getInstance()->saveSettings(
                    settings.hostname->text(),
                    settings.port->text(),
                    settings.rpcuser->text(),
                    settings.rpcpassword->text());

                auto cl = new ConnectionLoader(this, rpc);
                cl->loadConnection();
            }

            // Save explorer
            Settings::getInstance()->saveExplorer(
                settings.txExplorerUrl->text(),
                settings.addressExplorerUrl->text(),
                settings.onionTxExplorerUrl->text(),
                settings.onionAddressExplorerUrl->text());

            // Check to see if rescan or reindex have been enabled
            bool showRestartInfo = false;
            bool showReindexInfo = false;

            /*
            if (settings.chkRescan->isChecked()) {
                Settings::addToHushConf(hushConfLocation, "rescan=1");
                showRestartInfo = true;
            }*/

            if (settings.chkReindex->isChecked()) {
                Settings::addToHushConf(hushConfLocation, "reindex=1");
                showRestartInfo = true;
            }

             if (!rpc->getConnection()->config->consolidation.isEmpty()==false) {
                 if (settings.chkConso->isChecked()) {
                 Settings::addToHushConf(hushConfLocation, "consolidation=1");
                showRestartInfo = true;
                }
            }

            if (!rpc->getConnection()->config->consolidation.isEmpty()) {
                 if (settings.chkConso->isChecked() == false) {
                 Settings::removeFromHushConf(hushConfLocation, "consolidation");
                showRestartInfo = true;
                 }
            }

             if (!rpc->getConnection()->config->deletetx.isEmpty() == false) {
                 if (settings.chkDeletetx->isChecked()) {
                 Settings::addToHushConf(hushConfLocation, "deletetx=1");
                showRestartInfo = true;
                }
            }

             if (!rpc->getConnection()->config->deletetx.isEmpty()) {
                 if (settings.chkDeletetx->isChecked() == false) {
                 Settings::removeFromHushConf(hushConfLocation, "deletetx");
                showRestartInfo = true;
                 }
            }

             if (!rpc->getConnection()->config->zindex.isEmpty() == false) {
                 if (settings.chkzindex->isChecked()) {
                 Settings::addToHushConf(hushConfLocation, "zindex=1");
                 Settings::addToHushConf(hushConfLocation, "reindex=1");
                showReindexInfo = true;
                }
            }

             if (!rpc->getConnection()->config->zindex.isEmpty()) {
                 if (settings.chkzindex->isChecked() == false) {
                 Settings::removeFromHushConf(hushConfLocation, "zindex");
                 Settings::addToHushConf(hushConfLocation, "reindex=1");
                showReindexInfo = true;
                 }
            }

            if (showRestartInfo) {
                auto desc = tr("SilentDragon needs to restart to rescan,reindex,consolidation or deletetx. SilentDragon will now close, please restart SilentDragon to continue");

                QMessageBox::information(this, tr("Restart SilentDragon"), desc, QMessageBox::Ok);
                QTimer::singleShot(1, [=]() { this->close(); });
            }
            if (showReindexInfo) {
                auto desc = tr("SilentDragon needs to reindex for zindex. SilentDragon will now close, please restart SilentDragon to continue");

                QMessageBox::information(this, tr("Restart SilentDragon"), desc, QMessageBox::Ok);
                QTimer::singleShot(1, [=]() { this->close(); });
            }
        }
    });
}

void MainWindow::addressBook() {
    // Check to see if there is a target.
    QRegularExpression re("Address[0-9]+", QRegularExpression::CaseInsensitiveOption);
    for (auto target: ui->sendToWidgets->findChildren<QLineEdit *>(re)) {
        if (target->hasFocus()) {
            AddressBook::open(this, target);
            return;
        }
    };

    // If there was no target, then just run with no target.
    AddressBook::open(this);
}

void MainWindow::telegram() {
    QString url = "https://hush.is/tg";
    if (iszdeex) {
        url = "https://zdeex.com/tg";
    }
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::reportbug() {
    // zdeex doesn't have it's own support, for now
    QString url = "https://hush.is/tg_support";
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::website() {
    QString url = "https://hush.is";
    if (iszdeex) {
        url = "https://zdeex.com";
    }
    QDesktopServices::openUrl(QUrl(url));
}

// Validate an address
void MainWindow::validateAddress() {
    // Make sure everything is up and running
    if (!getRPC() || !getRPC()->getConnection())
        return;

    // First thing is ask the user for an address
    bool ok;
    auto address = QInputDialog::getText(this, tr("Enter Address to validate"),
        tr("Transparent or Shielded Address:") + QString(" ").repeated(140),    // Pad the label so the dialog box is wide enough
        QLineEdit::Normal, "", &ok);
    if (!ok)
        return;

    getRPC()->validateAddress(address, [=] (QJsonValue props) {
        QDialog d(this);
        Ui_ValidateAddress va;
        va.setupUi(&d);
        Settings::saveRestore(&d);
        Settings::saveRestoreTableHeader(va.tblProps, &d, "validateaddressprops");
        va.tblProps->horizontalHeader()->setStretchLastSection(true);

        va.lblAddress->setText(address);

        QList<QPair<QString, QString>> propsList;

        for (QString property_name: props.toObject().keys()) {

            QString property_value;

            if (props.toObject()[property_name].isString())
                property_value = props.toObject()[property_name].toString();
            else
                property_value = props.toObject()[property_name].toBool() ? "true" : "false" ;

            propsList.append(
                QPair<QString, QString>( property_name,
                                         property_value )
            );
        }

        ValidateAddressesModel model(va.tblProps, propsList);
        va.tblProps->setModel(&model);

        d.exec();
    });
}

// Get block info
void MainWindow::getBlock() {
    // Make sure everything is up and running
    if (!getRPC() || !getRPC()->getConnection())
        return;

    // First thing is ask the user for a block height
    bool ok;
    int i = QInputDialog::getInt(this, tr("Get Block Info"),
                                 tr("Enter Block Height:"), 1, 1, 2147483647, 1, &ok);
    if (!ok)
        return;

    auto blockheight = QString::number(i);

    getRPC()->getBlock(blockheight, [=] (QJsonValue props) {
        QDialog d(this);
        Ui_GetBlock gb;
        gb.setupUi(&d);
        Settings::saveRestore(&d);
        Settings::saveRestoreTableHeader(gb.tblProps, &d, "getblockprops");
        gb.tblProps->horizontalHeader()->setStretchLastSection(true);

        gb.lblHeight->setText(blockheight);

        QList<QPair<QString, QString>> propsList;

        for (QString property_name: props.toObject().keys()) {

            QString property_value;

            DEBUG("property = " << props.toObject()[property_name] );
            if (props.toObject()[property_name].isString()) {
                property_value = props.toObject()[property_name].toString();
            } else if (props.toObject()[property_name].isDouble()) {
                property_value = QString::number( props.toObject()[property_name].toDouble(), 'f', 0);
            } else if (props.toObject()[property_name].isBool()) {
                property_value = props.toObject()[property_name].toBool() ? "true" : "false" ;
            }

            propsList.append(
                QPair<QString, QString>( property_name,
                                         property_value )
            );
        }

        ValidateAddressesModel model(gb.tblProps, propsList);
        gb.tblProps->setModel(&model);

        d.exec();
    });
}

void MainWindow::doImport(QList<QString>* keys) {
    if (rpc->getConnection() == nullptr) {
        // No connection, just return
        return;
    }

    if (keys->isEmpty()) {
        delete keys;
        ui->statusBar->showMessage(tr("Private key import rescan finished"));
        return;
    }

    // Pop the first key
    QString key = keys->first();
    keys->pop_front();
    bool rescan = keys->isEmpty();

    if (key.startsWith("SK") ||
        key.startsWith("secret")) { // Z key
        rpc->importZPrivKey(key, rescan, [=] (auto) { this->doImport(keys); });
    } else {
        rpc->importTPrivKey(key, rescan, [=] (auto) { this->doImport(keys); });
    }
}


// Callback invoked when the RPC has finished loading all the balances, and the UI
// is now ready to send transactions.
void MainWindow::balancesReady() {
    // First-time check
    if (uiPaymentsReady)
        return;

    uiPaymentsReady = true;
    qDebug() << "Payment UI now ready!";

    // There is a pending URI payment (from the command line, or from a secondary instance),
    // process it.
    if (!pendingURIPayment.isEmpty()) {
        qDebug() << "Paying hush URI";
        payHushURI(pendingURIPayment);
        pendingURIPayment = "";
    }

}

// Event filter for MacOS specific handling of payment URIs
bool MainWindow::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->url().isEmpty())
            payHushURI(fileEvent->url().toString());

        return true;
    }

    return QObject::eventFilter(object, event);
}


// Pay the Hush URI by showing a confirmation window. If the URI parameter is empty, the UI
// will prompt for one. If the myAddr is empty, then the default from address is used to send
// the transaction.
void MainWindow::payHushURI(QString uri, QString myAddr) {
    // If the Payments UI is not ready (i.e, all balances have not loaded), defer the payment URI
    if (!uiPaymentsReady) {
        qDebug() << "Payment UI not ready, waiting for UI to pay URI";
        pendingURIPayment = uri;
        return;
    }

    // If there was no URI passed, ask the user for one.
    if (uri.isEmpty()) {
        uri = QInputDialog::getText(this, tr("Paste HUSH URI"),
            "HUSH URI" + QString(" ").repeated(180));
        if(iszdeex) {
            uri = QInputDialog::getText(this, tr("Paste ZDEEX URI"),
            "ZDEEX URI" + QString(" ").repeated(180));
        } 
    }

    // If there's no URI, just exit
    if (uri.isEmpty())
        return;

    // Extract the address
    qDebug() << "Received URI " << uri;
    PaymentURI paymentInfo = Settings::parseURI(uri);
    if (!paymentInfo.error.isEmpty()) {
        if(iszdeex) {
            QMessageBox::critical(this, tr("Error paying ZdeeX URI"),
                tr("URI should be of the form 'zdeex:<addr>?amt=x&memo=y") + "\n" + paymentInfo.error);
        } else {
            QMessageBox::critical(this, tr("Error paying Hush URI"),
                tr("URI should be of the form 'hush:<addr>?amt=x&memo=y") + "\n" + paymentInfo.error);
        }
        return;
    }

    // Now, set the fields on the send tab
    removeExtraAddresses();
    if (!myAddr.isEmpty()) {
        ui->inputsCombo->setCurrentText(myAddr);
    }

    ui->Address1->setText(paymentInfo.addr);
    ui->Address1->setCursorPosition(0);
    ui->Amount1->setText(Settings::getDecimalString(paymentInfo.amt.toDouble()));
    ui->MemoTxt1->setText(paymentInfo.memo);

    // And switch to the send tab.
    ui->tabWidget->setCurrentIndex(1);
    raise();

    // And click the send button if the amount is > 0, to validate everything. If everything is OK, it will show the confirm box
    // else, show the error message;
    if (paymentInfo.amt > 0) {
        sendButton();
    }
}


void MainWindow::importPrivKey() {
    QDialog d(this);
    Ui_PrivKey pui;
    pui.setupUi(&d);
    Settings::saveRestore(&d);

    pui.buttonBox->button(QDialogButtonBox::Save)->setVisible(false);
    pui.helpLbl->setText(QString() %
                        tr("Please paste your private keys here, one per line") % ".\n" %
                        tr("The keys will be imported into your connected Hush node"));

    if (d.exec() == QDialog::Accepted && !pui.privKeyTxt->toPlainText().trimmed().isEmpty()) {
        auto rawkeys = pui.privKeyTxt->toPlainText().trimmed().split("\n");

        QList<QString> keysTmp;
        // Filter out all the empty keys.
        std::copy_if(rawkeys.begin(), rawkeys.end(), std::back_inserter(keysTmp), [=] (auto key) {
            return !key.startsWith("#") && !key.trimmed().isEmpty();
        });

        auto keys = new QList<QString>();
        std::transform(keysTmp.begin(), keysTmp.end(), std::back_inserter(*keys), [=](auto key) {
            return key.trimmed().split(" ")[0];
        });

        // Special case.
        // Sometimes, when importing from a paperwallet or such, the key is split by newlines, and might have
        // been pasted like that. So check to see if the whole thing is one big private key
        if (Settings::getInstance()->isValidSaplingPrivateKey(keys->join(""))) {
            auto multiline = keys;
            keys = new QList<QString>();
            keys->append(multiline->join(""));
            delete multiline;
        }

        // Start the import. The function takes ownership of keys
        QTimer::singleShot(1, [=]() {doImport(keys);});

        // Show the dialog that keys will be imported.
        QMessageBox::information(this,
            "Imported", tr("The keys were imported! It may take several minutes to rescan the blockchain. Until then, functionality may be limited"),
            QMessageBox::Ok);
    }
}

/**
 * Export transaction history into a CSV file
 */
void MainWindow::exportTransactions() {
    // First, get the export file name
    QString exportName;
    if(iszdeex){
        exportName = "zdeex-transactions-" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".csv";
    }else{
        exportName = "hush-transactions-" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".csv";
    }

    QDir docsDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    QUrl pName = QUrl::fromLocalFile(docsDir.filePath(exportName));
    QUrl csvName = QFileDialog::getSaveFileUrl(this, tr("Export transactions"), pName, "CSV file (*.csv)");

    if (csvName.isEmpty())
        return;

    if (!rpc->getTransactionsModel()->exportToCsv(csvName.toLocalFile())) {
        QMessageBox::critical(this, tr("Error"),
            tr("Error exporting transactions, file was not saved"), QMessageBox::Ok);
    }
}

/**
 * Backup the wallet.dat file. This is kind of a hack, since it has to read from the filesystem rather than an RPC call
 * This might fail for various reasons - Remote hushd, non-standard locations, custom params passed to hushd, many others
*/
void MainWindow::backupWalletDat() {
    if (!rpc->getConnection())
        return;

    QDir hushdir(rpc->getConnection()->config->hushDir);
    QString backupDefaultName;
    if(iszdeex){
        backupDefaultName = "zdeex-wallet-backup-" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".dat";
    }else{
        backupDefaultName = "hush-wallet-backup-" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".dat";
    }

    if (Settings::getInstance()->isTestnet()) {
        hushdir.cd("testnet3");
        backupDefaultName = "testnet-" + backupDefaultName;
    }

    QFile wallet(hushdir.filePath("wallet.dat"));
    if (!wallet.exists()) {
        QMessageBox::critical(this, tr("No wallet.dat"), tr("Couldn't find the wallet.dat on this computer") + "\n" +
            tr("You need to back it up from the machine hushd is running on"), QMessageBox::Ok);
        return;
    }

    QUrl pName = QUrl::fromLocalFile(hushdir.filePath(backupDefaultName));
    QUrl backupName = QFileDialog::getSaveFileUrl(this, tr("Backup wallet.dat"), pName, "Data file (*.dat)");
    if (backupName.isEmpty())
        return;

    if (!wallet.copy(backupName.toLocalFile())) {
        QMessageBox::critical(this, tr("Couldn't backup"), tr("Couldn't backup the wallet.dat file.") +
            tr("You need to back it up manually."), QMessageBox::Ok);
    }
}

void MainWindow::exportAllKeys() {
    exportKeys("");
}

void MainWindow::getViewKey(QString addr) {
    QDialog d(this);
    Ui_ViewKey vui;
    vui.setupUi(&d);

    // Make the window big by default
    auto ps = this->geometry();
    QMargins margin = QMargins() + 50;
    d.setGeometry(ps.marginsRemoved(margin));

    Settings::saveRestore(&d);

    vui.viewKeyTxt->setPlainText(tr("Loading..."));
    vui.viewKeyTxt->setReadOnly(true);
    vui.viewKeyTxt->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);

    // Disable the save button until it finishes loading
    vui.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    vui.buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);

    bool allKeys = false; //addr.isEmpty() ? true : false;
    // Wire up save button
    QObject::connect(vui.buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, [=] () {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                           allKeys ? "hush-all-viewkeys.txt" : "hush-viewkey.txt");
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        QTextStream out(&file);
        // TODO: Output in address, viewkey CSV format?
        out << vui.viewKeyTxt->toPlainText();
    });

    auto isDialogAlive = std::make_shared<bool>(true);

    auto fnUpdateUIWithKeys = [=](QList<QPair<QString, QString>> viewKeys) {
        // Check to see if we are still showing.
        if (! *(isDialogAlive.get()) ) return;

        QString allKeysTxt;
        for (auto keypair : viewKeys) {
            allKeysTxt = allKeysTxt % keypair.second % " # addr=" % keypair.first % "\n";
        }

        vui.viewKeyTxt->setPlainText(allKeysTxt);
        vui.buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
    };

    auto fnAddKey = [=](QJsonValue key) {
        QList<QPair<QString, QString>> singleAddrKey;
        singleAddrKey.push_back(QPair<QString, QString>(addr, key.toString()));
        fnUpdateUIWithKeys(singleAddrKey);
    };

    rpc->getZViewKey(addr, fnAddKey);

    d.exec();
    *isDialogAlive = false;
}

void MainWindow::getQRCode(QString addr) {
    QDialog d(this);
    Ui_QRCode qrui;
    qrui.setupUi(&d);

    // Display QR Code for address
    qrui.qrcodeDisplayAddr->setQrcodeString(addr);

    // Set text/tip
    qrui.saveQRCodeBtn->setText(tr("Save"));
    qrui.saveQRCodeBtn->setToolTip(tr("Save QR Code to file"));

    auto isDialogAlive = std::make_shared<bool>(true);

    // Connect and handle Save button
    QObject::connect(qrui.saveQRCodeBtn, &QPushButton::clicked, [&] () {
        qDebug() << "Save QR Code clicked";
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save QR Code to file"), "",
            tr("Portable Network Graphics (*.png);;All Files (*)"));

        if (fileName.isEmpty())
            return;
        else {
            QFile file(fileName);
            // TODO: fix this. Saves as black image instead of QR code
            qrui.qrcodeDisplayAddr->grab().save(fileName, "png", -1);
            statusBar()->showMessage(tr("QR code saved"), 3000);
            d.close();
        }
    });

    d.exec();
    *isDialogAlive = false;
}

void MainWindow::exportKeys(QString addr) {
    bool allKeys = addr.isEmpty() ? true : false;

    QDialog d(this);
    Ui_PrivKey pui;
    pui.setupUi(&d);

    // Make the window big by default
    auto ps = this->geometry();
    QMargins margin = QMargins() + 50;
    d.setGeometry(ps.marginsRemoved(margin));

    Settings::saveRestore(&d);

    pui.privKeyTxt->setPlainText(tr("Loading..."));
    pui.privKeyTxt->setReadOnly(true);
    pui.privKeyTxt->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);

    if (allKeys)
        pui.helpLbl->setText(tr("These are all the private keys for all the addresses in your wallet"));
    else
        pui.helpLbl->setText(tr("Private key for ") + addr);

    // Disable the save button until it finishes loading
    pui.buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    pui.buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);

    // Wire up save button
    QObject::connect(pui.buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, [=] () {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                           allKeys ? "hush-all-privatekeys.txt" : "hush-privatekey.txt");
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        QTextStream out(&file);
        out << pui.privKeyTxt->toPlainText();
    });

    // Call the API
    auto isDialogAlive = std::make_shared<bool>(true);

    auto fnUpdateUIWithKeys = [=](QList<QPair<QString, QString>> privKeys) {
        // Check to see if we are still showing.
        if (! *(isDialogAlive.get()) ) return;

        QString allKeysTxt;
        for (auto keypair : privKeys) {
            allKeysTxt = allKeysTxt % keypair.second % " # addr=" % keypair.first % "\n";
        }

        pui.privKeyTxt->setPlainText(allKeysTxt);
        pui.buttonBox->button(QDialogButtonBox::Save)->setEnabled(true);
    };

    if (allKeys) {
        rpc->getAllPrivKeys(fnUpdateUIWithKeys);
    }
    else {
        auto fnAddKey = [=](QJsonValue key) {
            QList<QPair<QString, QString>> singleAddrKey;
            singleAddrKey.push_back(QPair<QString, QString>(addr, key.toString()));
            fnUpdateUIWithKeys(singleAddrKey);
        };

        if (Settings::getInstance()->isZAddress(addr)) {
            rpc->getZPrivKey(addr, fnAddKey);
        } else {
            rpc->getTPrivKey(addr, fnAddKey);
        }
    }

    d.exec();
    *isDialogAlive = false;
}

void MainWindow::setupBalancesTab() {
    ui->unconfirmedWarning->setVisible(false);

    // Double click on balances table
    auto fnDoSendFrom = [=](const QString& addr, const QString& to = QString(), bool sendMax = false) {
        // Find the inputs combo
        for (int i = 0; i < ui->inputsCombo->count(); i++) {
            auto inputComboAddress = ui->inputsCombo->itemText(i);
            if (inputComboAddress.startsWith(addr)) {
                ui->inputsCombo->setCurrentIndex(i);
                break;
            }
        }

        // If there's a to address, add that as well
        if (!to.isEmpty()) {
            // Remember to clear any existing address fields, because we are creating a new transaction.
            this->removeExtraAddresses();
            ui->Address1->setText(to);
        }

        // See if max button has to be checked
        if (sendMax) {
            ui->Max1->setChecked(true);
        }

        // And switch to the send tab.
        ui->tabWidget->setCurrentIndex(1);
    };

    // Double click opens up memo if one exists
    QObject::connect(ui->balancesTable, &QTableView::doubleClicked, [=](auto index) {
        index = index.sibling(index.row(), 0);
        auto addr = AddressBook::addressFromAddressLabel(ui->balancesTable->model()->data(index).toString());

        fnDoSendFrom(addr);
    });

    // Setup context menu on balances tab
    ui->balancesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->balancesTable, &QTableView::customContextMenuRequested, [=] (QPoint pos) {
        QModelIndex index = ui->balancesTable->indexAt(pos);
        if (index.row() < 0) return;

        index = index.sibling(index.row(), 0);
        auto addr = AddressBook::addressFromAddressLabel(
                            ui->balancesTable->model()->data(index).toString());

        QMenu menu(this);

        menu.addAction(tr("Copy address"), [=] () {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(addr);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

/*  Example reply from z_shieldcoinbase and z_mergetoaddress
{
  "remainingUTXOs": 0,
  "remainingValue": 0.00000000,
  "shieldingUTXOs": 6,
  "shieldingValue": 16.87530000,
  "opid": "opid-0245ddfa-5f60-4e00-8ace-e782d814132b"
}
*/

        if(addr.startsWith("zs1")) {
            menu.addAction(tr("Shield all non-mining taddr funds to this zaddr"), [=] () {
                QJsonArray params = QJsonArray { "ANY_TADDR" , addr };
                qDebug() << "Calling mergeToAddress with params=" << params;

                rpc->mergeToAddress(params, [=](const QJsonValue& reply) {
                    qDebug() << "mergeToAddress reply=" << reply;
                    QString shieldingValue = reply.toObject()["shieldingValue"].toString();
                    QString opid           = reply.toObject()["opid"].toString();
                    auto    remainingUTXOs = reply.toObject()["remainingUTXOs"].toInt();
                    if(remainingUTXOs > 0) {
                       //TODO: more utxos to shield
                    }

                    ui->statusBar->showMessage(tr("Shielded") + shieldingValue + " HUSH in transparent funds to " + addr + " in opid " + opid, 3 * 1000);
                }, [=](QString errStr) {
                    qDebug() << "z_mergetoaddress pooped:" << errStr;
                    if(errStr == "Could not find any funds to merge.") {
                        ui->statusBar->showMessage("No funds found to shield!");
                    }
                });

            });
        }

        if(addr.startsWith("zs1")) {
        menu.addAction(tr("Shield all mining funds to this zaddr"), [=] () {
            //QJsonArray params = QJsonArray {addr, zaddresses->first() };
            // We shield all coinbase funds to the selected zaddr
            QJsonArray params = QJsonArray {"*", addr };

            qDebug() << "Calling shieldCoinbase with params=" << params;
            rpc->shieldCoinbase(params, [=](const QJsonValue& reply) {
                QString shieldingValue = reply.toObject()["shieldingValue"].toString();
                QString opid           = reply.toObject()["opid"].toString();
                auto    remainingUTXOs = reply.toObject()["remainingUTXOs"].toInt();
                qDebug() << "ShieldCoinbase reply=" << reply;
                // By default we shield 50 blocks at a time
                if(remainingUTXOs > 0) {
                   //TODO: more utxos to shield
                }
                ui->statusBar->showMessage(tr("Shielded") + shieldingValue + " HUSH in Mining funds to " + addr + " in opid " + opid, 3 * 1000);
            }, [=](QString errStr) {
                //error("", errStr);
                qDebug() << "z_shieldcoinbase pooped:" << errStr;
                if(errStr == "Could not find any coinbase funds to shield.") {
                    ui->statusBar->showMessage("No mining funds found to shield!");
                }
            });

        });
        }

        menu.addAction(tr("Get private key"), [=] () {
            this->exportKeys(addr);
        });

        if (addr.startsWith("zs1")) {
            menu.addAction(tr("Get viewing key"), [=] () {
                this->getViewKey(addr);
            });

            // QR Code for zaddrs only
            menu.addAction(tr("Get QR code"), [=] () {
                this->getQRCode(addr);
            });
        }

        menu.addAction("Send from " % addr.left(40) % (addr.size() > 40 ? "..." : ""), [=]() {
            fnDoSendFrom(addr);
        });

        menu.addAction("Send to " % addr.left(40) % (addr.size() > 40 ? "..." : ""), [=]() {
            fnDoSendFrom("",addr);
        });

        if (addr.startsWith("R")) {
            auto defaultSapling = rpc->getDefaultSaplingAddress();
            if (!defaultSapling.isEmpty()) {
                menu.addAction(tr("Shield balance to Sapling"), [=] () {
                    fnDoSendFrom(addr, defaultSapling, true);
                });
            }

            menu.addAction(tr("Shield mining funds to default zaddr"), [=] () {
                auto defaultZaddr = rpc->getDefaultSaplingAddress();
                QJsonArray params = QJsonArray {addr, defaultZaddr };
                qDebug() << "Calling shieldCoinbase with params=" << params;
                rpc->shieldCoinbase(params, [=](const QJsonValue& reply) {
                    QString shieldingValue = reply.toObject()["shieldingValue"].toString();
                    QString opid           = reply.toObject()["opid"].toString();
                    auto    remainingUTXOs = reply.toObject()["remainingUTXOs"].toInt();
                    qDebug() << "ShieldCoinbase reply=" << reply;
                    // By default we shield 50 blocks at a time
                    if(remainingUTXOs > 0) {
                       //TODO: more utxos to shield
                    }
                    ui->statusBar->showMessage(tr("Shielded") + shieldingValue + " HUSH in Mining funds to " + addr + " in opid " + opid, 3 * 1000);
                }, [=](QString errStr) {
                    //error("", errStr);
                    qDebug() << "z_shieldcoinbase pooped:" << errStr;
                    if(errStr == "Could not find any coinbase funds to shield.") {
                        ui->statusBar->showMessage("No mining funds found to shield!");
                    }
                });
            });

            menu.addAction(tr("View on block explorer"), [=] () {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.addressExplorerUrl + addr;
                DEBUG("explorer url=" << url);
                QDesktopServices::openUrl(QUrl(url));
            });

            menu.addAction(tr("View on Tor block explorer"), [=] () {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.onionAddressExplorerUrl + addr;
                DEBUG("explorer url=" << url);
                QDesktopServices::openUrl(QUrl(url));
            });

            menu.addAction("Copy explorer link", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.addressExplorerUrl + addr;
                DEBUG("explorer url=" << url);
                QGuiApplication::clipboard()->setText(url);
            });

            menu.addAction("Copy Tor explorer link", [=]() {
                QString url;
                auto explorer = Settings::getInstance()->getExplorer();
                url = explorer.onionAddressExplorerUrl + addr;
                QGuiApplication::clipboard()->setText(url);
            });

            menu.addAction(tr("Address Asset Viewer"), [=] () {
                QString url;
                url = "https://dexstats.info/assetviewer.php?address=" + addr;
                QDesktopServices::openUrl(QUrl(url));
            });

            //TODO: should this be kept?
            menu.addAction(tr("Convert Address"), [=] () {
                QString url;
                // HUSH3 can be used for all HSC's since they all have the same address format
                url = "https://dexstats.info/addressconverter.php?fromcoin=HUSH3&address=" + addr;
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        menu.exec(ui->balancesTable->viewport()->mapToGlobal(pos));
    });
}

QString peer2ip(QString peer) {
    QString ip = "";
    if(peer.contains("[")) {
        // this is actually ipv6, grab it all except the port
        auto parts = peer.split(":");
        parts[8]=""; // remove  port
        peer = parts.join(":");
        peer.chop(1); // remove trailing :
    } else {
        ip     = peer.split(":")[0];
    }
    return ip;
}

void MainWindow::setupMiningTab() {
    DEBUG("setting up mining tab");
    //TODO: for other HSC's, look at getinfo.ac_algo == randomx
    if(iszdeex) {
        int hwc = std::thread::hardware_concurrency();
        DEBUG("hardware concurrency = " << hwc);
        auto tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("Mining"));

        ui->tabWidget->addTab(tab, QString(tr("Mining")));
        auto gridLayout = new QGridLayout(tab);
        gridLayout->setSpacing(6);
        //auto label1     = new QLabel(tr("Threads"), tab);
        auto label2     = new QLabel(tr("Mining threads"), tab);
        auto label3     = new QLabel(tr("Local Hashrate (hashes/sec)"), tab);
        auto label4     = new QLabel(tr("Network Hashrate (hashes/sec)"), tab);
        auto label5     = new QLabel(tr("Difficulty"), tab);
        auto label6     = new QLabel(tr("Estimated Hours To Find A Block"), tab);
        auto label7     = new QLabel(tr("Select the number of threads to mine with:"), tab);
        label2->setObjectName("mininglabel2");
        label3->setObjectName("mininglabel3");
        label4->setObjectName("mininglabel4");
        label5->setObjectName("mininglabel5");
        label6->setObjectName("mininglabel6");
        label7->setObjectName("mininglabel7");

        auto combo      = new QComboBox(tab);
        combo->setObjectName("genproclimit");

        auto threadStr  = tr("thread");
        auto threadsStr = tr("threads");
        // give options from 1 to hwc/2 , which should represent physical CPUs
        for(int i=0; i < hwc/2; i++) {
            combo->insertItem(i, QString::number(i+1) % " " % (i+1==1 ? threadStr : threadsStr));
        }

        QFont font;
        font.setBold(true);
        font.setPointSize(18);
        // probably a better way to do this but yolo
        label2->setFont(font);
        label3->setFont(font);
        label4->setFont(font);
        label5->setFont(font);
        label6->setFont(font);
        label7->setFont(font);
        label7->setAlignment(Qt::AlignHCenter);
        auto lcd1       = new QLCDNumber(8, tab);
        auto lcd2       = new QLCDNumber(8, tab);
        auto lcd3       = new QLCDNumber(8, tab);
        auto lcd4       = new QLCDNumber(8, tab);
        auto lcd5       = new QLCDNumber(8, tab);
        lcd1->display(QString("0.0"));
        lcd1->setObjectName("localhashrate");

        lcd2->display(QString("0"));
        lcd2->setObjectName("networkhashrate");

        lcd3->display(QString("0.0"));
        lcd3->setObjectName("difficulty");

        lcd4->display(QString("-"));
        lcd4->setObjectName("luck");

        lcd5->display(QString("0"));
        lcd5->setObjectName("miningthreads");

        auto button1   = new QPushButton(tr("Start Mining"), tab);
        auto button2   = new QPushButton(tr("Stop Mining"), tab);
        button1->setFont(font);
        button2->setFont(font);
        button1->setObjectName("startmining");
        button2->setObjectName("stopmining");

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
            auto button1 = ui->tabWidget->findChild<QPushButton *>("startmining");
            if(button1 != nullptr) {
                DEBUG("found start mining button and enabled=" << button1->isEnabled() );

                if(button1->isEnabled()) {
                    // if start mining button is enabled, we are not currently mining, so do nothing
                    return;
                }
            }

            // if we are currently mining and thread count combo changes, change our genproclimit
            // to that number of threads, instead of users have to stop and restart mining, which
            // is dumb and non-intuitive

            DEBUG("combobox changed index=" << index);
            int threads = index+1;
            DEBUG("changing number of threads to " << threads);
            rpc->setGenerate(threads, [=] (QJsonValue response){
                DEBUG("setgenerate response=" << response);

                // instantly update miningthreads GUI
                auto miningthreads = ui->tabWidget->findChild<QLCDNumber *>("miningthreads");
                miningthreads->display(QString::number(threads)); // miningthreads
                DEBUG("updated mining thread count to " << QString(threads) );
            });
        });

        QObject::connect(button1, &QPushButton::clicked, [&] () {
            DEBUG("START MINING");
            int threads = 1;

            auto combo = ui->tabWidget->findChild<QComboBox *>("genproclimit");
            if(combo != nullptr) {
                DEBUG("found combo with selection index=" << combo->currentIndex() );
                threads = combo->currentIndex() + 1;
            }
            ui->statusBar->showMessage(tr("Starting mining with ") + QString::number(threads) + tr(" threads"), 5000);

            rpc->setGenerate(threads, [=] (QJsonValue response){
                DEBUG("setgenerate response=" << response);
                // these values will auto-update in a few seconds but do it
                // immediately so as to not confuse the user
                auto miningthreads = ui->tabWidget->findChild<QLCDNumber *>("miningthreads");
                // miningthreads->display(QString(threads)); // miningthreads
                miningthreads->display(QString::number(threads)); // miningthreads
                DEBUG("updated mining thread count to " << QString(threads) );
            });
        });

        QObject::connect(button2, &QPushButton::clicked, [&] () {
            DEBUG("STOP MINING");
            ui->statusBar->showMessage(tr("Stopping mining..."), 5000);
            rpc->stopGenerate(0, [=] (QJsonValue response){
                DEBUG("setgenerate response=" << response);
                // these values will auto-update in a few seconds but do it
                // immediately so as to not confuse the user
                // TODO: coredumps
                // lcd1->display(QString("0")); // localhash
                // lcd4->display(QString("0")); // luck
                // lcd5->display(QString("0")); // miningthreads
            });
        });


        // both buttons disabled at creation time. when we know the current
        // status of getmininginfo.generate we enable the correct button
        button1->setEnabled(false);
        button2->setEnabled(false);

        // gridLayout->addWidget(radio, 0, 0);
        // gridLayout->addWidget(label1, 0, 1, Qt::AlignLeft);
        gridLayout->addWidget(button1, 0, 0);
        //gridLayout->addWidget(label1, 0, 1);
        gridLayout->addWidget(label7, 0, 1);
        gridLayout->addWidget(button2, 1, 0);
        gridLayout->addWidget(combo, 1, 1);
        gridLayout->addWidget(label2, 2, 0);
        gridLayout->addWidget(lcd5, 2, 1);
        gridLayout->addWidget(label3, 3, 0);
        gridLayout->addWidget(lcd1, 3, 1);
        gridLayout->addWidget(label4, 4, 0);
        gridLayout->addWidget(lcd2, 4, 1);
        gridLayout->addWidget(label5, 5, 0);
        gridLayout->addWidget(lcd3, 5, 1);
        gridLayout->addWidget(label6, 6, 0);
        gridLayout->addWidget(lcd4, 6, 1);
    } else {
        // Mining tab currently only enabled for ZdeeX
    }
}
void MainWindow::setupPeersTab() {
    qDebug() << __FUNCTION__;
    // Set up context menu on peers tab
    ui->peersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->bannedPeersTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // Table right click
    QObject::connect(ui->bannedPeersTable, &QTableView::customContextMenuRequested, [=] (QPoint pos) {
        QModelIndex index = ui->bannedPeersTable->indexAt(pos);
        if (index.row() < 0) return;

        QMenu menu(this);

        auto bannedPeerModel = dynamic_cast<BannedPeersTableModel *>(ui->bannedPeersTable->model());
        QString addr         = bannedPeerModel->getAddress(index.row());
        qint64 asn           = bannedPeerModel->getASN(index.row());
        QString ip           = peer2ip(addr);
        QString subnet       = bannedPeerModel->getSubnet(index.row());
        QString as           = QString::number(asn);
        //qint64 banned_until  = bannedPeerModel->getBannedUntil(index.row());

        if(!ip.isEmpty()) {
            menu.addAction(tr("Copy banned peer IP"), [=] () {
                QGuiApplication::clipboard()->setText(ip);
                ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
            });
        }

        // shodan only supports ipv4 addresses *and* we get ipv6 addresses
        // in a different format, yay
        if(!ip.isEmpty() && !ip.contains(":")) {
            menu.addAction(tr("View banned host IP on shodan.io (3rd party service)"), [=] () {
                QString url = "https://www.shodan.io/host/" + ip;
                qDebug() << "opening " << url;
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        if(!as.isEmpty()) {
            menu.addAction(tr("View ASN on bgpview.io (3rd party service)"), [=] () {
                QString url = "https://bgpview.io/asn/" + as;
                qDebug() << "opening " << url;
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        if(!ip.isEmpty()) {
            menu.addAction(tr("Unban this peer"), [=] () {
                ui->statusBar->showMessage(tr("Unbanning peer..."));

                // Hide single banned peer
                ui->bannedPeersTable->hideRow(index.row());

                // Call setban
                rpc->setban(ip, "remove", [=] (QJsonValue response){
                    qDebug() << "setban remove " << response;
                    ui->statusBar->showMessage(tr("Peer unbanned"), 3 * 1000);
                    rpc->refreshPeers();
                });
            });

            menu.addAction(tr("Unban all peers"), [=] () {
                ui->statusBar->showMessage(tr("Unbanning all peers..."));

                // Hide all banned peers
                for (int i=0; i < bannedPeerModel->rowCount(index); i++){
                    ui->bannedPeersTable->hideRow(i);
                }

                // Call clearBanned
                rpc->clearBanned([=] (QJsonValue response){
                    qDebug() << "clearBanned " << response;
                    ui->statusBar->showMessage(tr("All peers unbanned"), 3 * 1000);
                    rpc->refreshPeers();
                });
            });
        }

        menu.exec(ui->bannedPeersTable->viewport()->mapToGlobal(pos));
    });

    // Table right click
    QObject::connect(ui->peersTable, &QTableView::customContextMenuRequested, [=] (QPoint pos) {
        QModelIndex index = ui->peersTable->indexAt(pos);
        if (index.row() < 0) return;

        QMenu menu(this);

        auto peerModel = dynamic_cast<PeersTableModel *>(ui->peersTable->model());
        QString addr   = peerModel->getAddress(index.row());
        QString cipher = peerModel->getTLSCipher(index.row());
        qint64 asn     = peerModel->getASN(index.row());
        QString ip     = peer2ip(addr);
        QString as     = QString::number(asn);

        menu.addAction(tr("Copy peer address+port"), [=] () {
            QGuiApplication::clipboard()->setText(addr);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

        //TODO: support Tor correctly when v3 lands
        menu.addAction(tr("Copy peer address"), [=] () {
            QGuiApplication::clipboard()->setText(ip);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

        menu.addAction(tr("Copy TLS ciphersuite"), [=] () {
            QGuiApplication::clipboard()->setText(cipher);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

        menu.addAction(tr("Copy ASN"), [=] () {
            QGuiApplication::clipboard()->setText(as);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

        // shodan only supports ipv4 addresses
        if(!ip.isEmpty() && !ip.contains("[")) {
            menu.addAction(tr("View host on shodan.io (3rd party service)"), [=] () {
                QString url = "https://www.shodan.io/host/" + ip;
                qDebug() << "opening " << url;
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        if(!as.isEmpty()) {
            menu.addAction(tr("View ASN on bgpview.io (3rd party service)"), [=] () {
                QString url = "https://bgpview.io/asn/" + as;
                qDebug() << "opening " << url;
                QDesktopServices::openUrl(QUrl(url));
            });
        }

        menu.addAction(tr("Ban this peer"), [=] () {
            ui->statusBar->showMessage(tr("Banning peer..."));

            // Hide single peer
            ui->peersTable->hideRow(index.row());

            // Call setban
            rpc->setban(ip, "add", [=] (QJsonValue response){
                qDebug() << "setban add " << response;
                ui->statusBar->showMessage(tr("Peer banned"), 3 * 1000);
                rpc->refreshPeers();
            });
        });

        menu.exec(ui->peersTable->viewport()->mapToGlobal(pos));
    });

    /*
    //grep 'BAN THRESHOLD EXCEEDED' ~/.hush/HUSH3/debug.log
    //grep Disconnected ...
    QFile debuglog = "";

#ifdef Q_OS_LINUX
    debuglog = "~/.hush/HUSH3/debug.log";
#elif defined(Q_OS_DARWIN)
    debuglog = "~/Library/Application Support/Hush/HUSH3/debug.log";
#elif defined(Q_OS_WIN64)
    // "C:/Users/<USER>/AppData/Roaming/<APPNAME>",
    // TODO: get current username
    debuglog = "C:/Users/<USER>/AppData/Roaming/Hush/HUSH3/debug.log";
#else
    // Bless Your Heart, You Like Danger!
    // There are open bounties to port HUSH softtware to OpenBSD and friends:
    // git.hush.is/hush/tasks
    debuglog = "~/.hush/HUSH3/debug.log";
#endif // Q_OS_LINUX

    if(debuglog.exists()) {
        qDebug() << __func__ << ": Found debuglog at " << debuglog;
    } else {
        qDebug() << __func__ << ": No debug.log found";
    }
    */

    //ui->recentlyBannedPeers = "Could not open " + debuglog;

}

void MainWindow::setupHushTab() {
    QPixmap image(":/img/hushdlogo.png");
    ui->hushlogo->setBasePixmap( image ); // image.scaled(600,600,  Qt::KeepAspectRatioByExpanding, Qt::FastTransformation ) );
}
/*
void MainWindow::setupChatTab() {
    qDebug() << __FUNCTION__;
    QList<QPair<QString,QString>> addressLabels = AddressBook::getInstance()->getAllAddressLabels();
    QStringListModel *chatModel = new QStringListModel();
    QStringList contacts;
    //contacts << "Alice" << "Bob" << "Charlie" << "Eve";
    for (int i = 0; i < addressLabels.size(); ++i) {
        QPair<QString,QString> pair = addressLabels.at(i);
        qDebug() << "Found contact " << pair.first << " " << pair.second;
        contacts << pair.first;
    }

    chatModel->setStringList(contacts);

    QStringListModel *conversationModel = new QStringListModel();
    QStringList conversations;
    conversations << "Bring home some milk" << "Markets look rough" << "How's the weather?" << "Is this on?";
    conversationModel->setStringList(conversations);


    //Ui_addressBook ab;
    //AddressBookModel model(ab.addresses);
    //ab.addresses->setModel(&model);

    //TODO: ui->contactsView->setModel( model of address book );
    //ui->contactsView->setModel(&model );

    ui->contactsView->setModel(chatModel);
    ui->chatView->setModel( conversationModel );
}
*/

void MainWindow::setupMarketTab() {
    qDebug() << "Setting up market tab";
    auto s      = Settings::getInstance();
    auto ticker = s->get_currency_name();

    ui->volume->setText(QString::number((double)       s->get_volume("HUSH") ,'f',8) + " HUSH");
    ui->volumeLocal->setText(QString::number((double)  s->get_volume(ticker) ,'f',8) + " " + ticker);
    ui->volumeBTC->setText(QString::number((double)    s->get_volume("BTC") ,'f',8) + " BTC");
}

void MainWindow::setupTransactionsTab() {
    // Double click opens up memo if one exists
    QObject::connect(ui->transactionsTable, &QTableView::doubleClicked, [=] (auto index) {
        auto txModel = dynamic_cast<TxTableModel *>(ui->transactionsTable->model());
        QString memo = txModel->getMemo(index.row());

        if (!memo.isEmpty()) {
            QMessageBox mb;
            mb.setText(memo);
            mb.setWindowTitle(tr("Memo"));
            mb.setIcon(QMessageBox::Information);

            QAbstractButton* buttonMemoReply = mb.addButton(tr("Reply"), QMessageBox::YesRole); mb.addButton(tr("OK"), QMessageBox::NoRole);

            mb.setTextFormat(Qt::PlainText);
            mb.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            mb.exec();

            if (mb.clickedButton()==buttonMemoReply) {
                qDebug() << "Reply clicked";

                int lastPost     = memo.trimmed().lastIndexOf(QRegExp("[\r\n]+"));
                QString lastWord = memo.right(memo.length() - lastPost - 1);

                if (Settings::getInstance()->isSaplingAddress(lastWord)) {

                    // First, cancel any pending stuff in the send tab by pretending to click
                    // the cancel button
                    cancelButton();

                    // Then set up the fields in the send tab
                    ui->Address1->setText(lastWord);
                    ui->Address1->setCursorPosition(0);
                    ui->Amount1->setText("0.0001");

                    // And switch to the send tab.
                    ui->tabWidget->setCurrentIndex(1);

                    qApp->processEvents();

                    // Click the memo button
                    this->memoButtonClicked(1, true);
                }else{
                    // TODO: This memo has no reply to address. Show alert or don't show button to begin with.
                    QMessageBox mb;
                    mb.setText(tr("Sorry! This memo has no reply to address."));
                    mb.setWindowTitle(tr("Error"));

                    mb.setTextFormat(Qt::PlainText);
                    mb.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
                    mb.exec();
                }
            }
        }
    });

    // Set up context menu on transactions tab
    ui->transactionsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // Table right click
    QObject::connect(ui->transactionsTable, &QTableView::customContextMenuRequested, [=] (QPoint pos) {
        QModelIndex index = ui->transactionsTable->indexAt(pos);
        if (index.row() < 0) return;

        QMenu menu(this);

        auto txModel = dynamic_cast<TxTableModel *>(ui->transactionsTable->model());

        QString txid = txModel->getTxId(index.row());
        QString memo = txModel->getMemo(index.row());
        QString addr = txModel->getAddr(index.row());

        menu.addAction(tr("Copy txid"), [=] () {
            QGuiApplication::clipboard()->setText(txid);
            ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
        });

        if (!addr.isEmpty()) {
            menu.addAction(tr("Copy address"), [=] () {
                QGuiApplication::clipboard()->setText(addr);
                ui->statusBar->showMessage(tr("Copied to clipboard"), 3 * 1000);
            });
        }

        menu.addAction(tr("View on block explorer"), [=] () {
            QString url;
            auto explorer = Settings::getInstance()->getExplorer();
            url = explorer.txExplorerUrl + txid;
            DEBUG("explorer url=" << url);
            QDesktopServices::openUrl(QUrl(url));
        });

        menu.addAction(tr("View on Tor block explorer"), [=] () {
            QString url;
            auto explorer = Settings::getInstance()->getExplorer();
            url = explorer.onionTxExplorerUrl + txid;
            DEBUG("explorer url=" << url);
            QDesktopServices::openUrl(QUrl(url));
        });

        menu.addAction(tr("Copy block explorer link"), [=] () {
            QString url;
            auto explorer = Settings::getInstance()->getExplorer();
            url = explorer.txExplorerUrl + txid;
            DEBUG("explorer url=" << url);
            QGuiApplication::clipboard()->setText(url);
        });

        menu.addAction(tr("Copy Tor block explorer link"), [=] () {
            QString url;
            auto explorer = Settings::getInstance()->getExplorer();
            url = explorer.onionTxExplorerUrl + txid;
            DEBUG("explorer url=" << url);
            QGuiApplication::clipboard()->setText(url);
        });

        /* TODO: Decide whether to use this or not.
        menu.addAction(tr("Look for new transactions"), [=] () {
            QGuiApplication::clipboard()->setText(addr);
            ui->statusBar->showMessage(tr("Looking for new transactions"), 3 * 1000);
            rpc->watchTxStatus();
        });
        */

        // Payment Request
        if (!memo.isEmpty() && memo.startsWith(iszdeex ? "zdeex:" : "hush:")) {
            menu.addAction(tr("View Payment Request"), [=] () {
                RequestDialog::showPaymentConfirmation(this, memo);
            });
        }

        // View Memo
        if (!memo.isEmpty()) {
            menu.addAction(tr("View Memo"), [=] () {
                /*
                QMessageBox mb(QMessageBox::Information, tr("Memo"), memo, QMessageBox::Ok, this);
                mb.setTextFormat(Qt::PlainText);
                mb.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
                mb.exec();*/
                QMessageBox mb;
                mb.setText(memo);
                mb.setWindowTitle(tr("Memo"));
                mb.setIcon(QMessageBox::Information);

                QAbstractButton* buttonMemoReply = mb.addButton(tr("Reply"), QMessageBox::YesRole); mb.addButton(tr("OK"), QMessageBox::NoRole);

                mb.setTextFormat(Qt::PlainText);
                mb.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
                mb.exec();

                if (mb.clickedButton()==buttonMemoReply) {
                    qDebug() << "Reply clicked";

                    int lastPost     = memo.trimmed().lastIndexOf(QRegExp("[\r\n]+"));
                    QString lastWord = memo.right(memo.length() - lastPost - 1);

                    if (Settings::getInstance()->isSaplingAddress(lastWord)) {

                        // First, cancel any pending stuff in the send tab by pretending to click
                        // the cancel button
                        cancelButton();

                        // Then set up the fields in the send tab
                        ui->Address1->setText(lastWord);
                        ui->Address1->setCursorPosition(0);
                        ui->Amount1->setText("0.0001");

                        // And switch to the send tab.
                        ui->tabWidget->setCurrentIndex(1);

                        qApp->processEvents();

                        // Click the memo button
                        this->memoButtonClicked(1, true);
                    }else{
                        // TODO: This memo has no reply to address. Show alert or don't show button to begin with.
                        QMessageBox mb;
                        mb.setText(tr("Sorry! This memo has no reply to address."));
                        mb.setWindowTitle(tr("Error"));

                        mb.setTextFormat(Qt::PlainText);
                        mb.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
                        mb.exec();
                    }
                }
            });
        }

        // If memo contains a reply to address, add a "Reply to" menu item
        if (!memo.isEmpty()) {
            int lastPost     = memo.trimmed().lastIndexOf(QRegExp("[\r\n]+"));
            QString lastWord = memo.right(memo.length() - lastPost - 1);

            if (Settings::getInstance()->isSaplingAddress(lastWord)) {
                menu.addAction(tr("Reply to ") + lastWord.left(25) + "...", [=]() {
                    // First, cancel any pending stuff in the send tab by pretending to click
                    // the cancel button
                    cancelButton();

                    // Then set up the fields in the send tab
                    ui->Address1->setText(lastWord);
                    ui->Address1->setCursorPosition(0);
                    ui->Amount1->setText("0.0001");

                    // And switch to the send tab.
                    ui->tabWidget->setCurrentIndex(1);

                    qApp->processEvents();

                    // Click the memo button
                    this->memoButtonClicked(1, true);
                });
            }
        }

        menu.exec(ui->transactionsTable->viewport()->mapToGlobal(pos));
    });
}

void MainWindow::addNewZaddr() {
    rpc->newZaddr( [=] (QJsonValue reply) {
        QString addr = reply.toString();
        // Make sure the RPC class reloads the z-addrs for future use
        rpc->refreshAddresses();

        // Just double make sure the z-address is still checked
        if ( ui->rdioZSAddr->isChecked() ) {
            ui->listReceiveAddresses->insertItem(0, addr);
            ui->listReceiveAddresses->setCurrentIndex(0);

            ui->statusBar->showMessage(QString::fromStdString("Created new Sapling zaddr"), 10 * 1000);
        }
    });
}


// Adds z-addresses to the combo box. Technically, returns a
// lambda, which can be connected to the appropriate signal
std::function<void(bool)> MainWindow::addZAddrsToComboList(bool sapling) {
    return [=] (bool checked) {
        if (checked && this->rpc->getAllZAddresses() != nullptr) {
            auto addrs = this->rpc->getAllZAddresses();
            ui->listReceiveAddresses->clear();

            std::for_each(addrs->begin(), addrs->end(), [=] (auto addr) {
                if ( (sapling &&  Settings::getInstance()->isSaplingAddress(addr)) ||
                    (!sapling && !Settings::getInstance()->isSaplingAddress(addr))) {
                        if (rpc->getAllBalances()) {
                            auto bal = rpc->getAllBalances()->value(addr);
                            ui->listReceiveAddresses->addItem(addr, bal);
                        }
                }
            });

            // If z-addrs are empty, then create a new one.
            if (addrs->isEmpty()) {
                addNewZaddr();
            }
        }
    };
}

void MainWindow::setupReceiveTab() {
    auto addNewTAddr = [=] () {
        rpc->newTaddr([=] (QJsonValue reply) {
            qDebug() << "New addr button clicked";
            QString addr = reply.toString();
            // Make sure the RPC class reloads the t-addrs for future use
            rpc->refreshAddresses();

            // Just double make sure the t-address is still checked
            if (ui->rdioTAddr->isChecked()) {
                ui->listReceiveAddresses->insertItem(0, addr);
                ui->listReceiveAddresses->setCurrentIndex(0);

                ui->statusBar->showMessage(tr("Created new t-Addr"), 10 * 1000);
            }
        });
    };

    // Connect t-addr radio button
    QObject::connect(ui->rdioTAddr, &QRadioButton::toggled, [=] (bool checked) {
        qDebug() << "taddr radio toggled";
        if (checked && this->rpc->getUTXOs() != nullptr) {
            updateTAddrCombo(checked);
        }

        // Toggle the "View all addresses" button as well
        ui->btnViewAllAddresses->setVisible(checked);
    });

    // View all addresses goes to "View all private keys"
    QObject::connect(ui->btnViewAllAddresses, &QPushButton::clicked, [=] () {
        // If there's no RPC, return
        if (!getRPC())
            return;

        QDialog d(this);
        Ui_ViewAddressesDialog viewaddrs;
        viewaddrs.setupUi(&d);
        Settings::saveRestore(&d);
        Settings::saveRestoreTableHeader(viewaddrs.tblAddresses, &d, "viewalladdressestable");
        viewaddrs.tblAddresses->horizontalHeader()->setStretchLastSection(true);

        ViewAllAddressesModel model(viewaddrs.tblAddresses, *getRPC()->getAllTAddresses(), getRPC());
        viewaddrs.tblAddresses->setModel(&model);

        QObject::connect(viewaddrs.btnExportAll, &QPushButton::clicked,  this, &MainWindow::exportAllKeys);

        viewaddrs.tblAddresses->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(viewaddrs.tblAddresses, &QTableView::customContextMenuRequested, [=] (QPoint pos) {
            QModelIndex index = viewaddrs.tblAddresses->indexAt(pos);
            if (index.row() < 0) return;

            index = index.sibling(index.row(), 0);
            QString addr = viewaddrs.tblAddresses->model()->data(index).toString();

            QMenu menu(this);
            menu.addAction(tr("Export Private Key"), [=] () {
                if (addr.isEmpty())
                    return;

                this->exportKeys(addr);
            });
            menu.addAction(tr("Copy Address"), [=]() {
                QGuiApplication::clipboard()->setText(addr);
            });
            menu.exec(viewaddrs.tblAddresses->viewport()->mapToGlobal(pos));
        });

        d.exec();
    });

    QObject::connect(ui->rdioZSAddr, &QRadioButton::toggled, addZAddrsToComboList(true));

    // Explicitly get new address button.
    QObject::connect(ui->btnReceiveNewAddr, &QPushButton::clicked, [=] () {
        if (!rpc->getConnection())
            return;

        if (ui->rdioZSAddr->isChecked()) {
            addNewZaddr();
        } else if (ui->rdioTAddr->isChecked()) {
            addNewTAddr();
        }
    });

    // Focus enter for the Receive Tab
    QObject::connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (int tab) {
        if (tab == 2) {
            // Switched to receive tab, select the z-addr radio button
            ui->rdioZSAddr->setChecked(true);
            ui->btnViewAllAddresses->setVisible(false);

            // And then select the first one
            ui->listReceiveAddresses->setCurrentIndex(0);
        }
    });

    // Validator for label
    QRegExpValidator* v = new QRegExpValidator(QRegExp(Settings::labelRegExp), ui->rcvLabel);
    ui->rcvLabel->setValidator(v);

    // Select item in address list
    QObject::connect(ui->listReceiveAddresses,
        QOverload<int>::of(&QComboBox::currentIndexChanged), [=] (int index) {
        QString addr = ui->listReceiveAddresses->itemText(index);
        if (addr.isEmpty()) {
            // Draw empty stuff

            ui->rcvLabel->clear();
            ui->rcvBal->clear();
            ui->txtReceive->clear();
            ui->qrcodeDisplay->clear();
            return;
        }

        auto label = AddressBook::getInstance()->getLabelForAddress(addr);
        if (label.isEmpty()) {
            ui->rcvUpdateLabel->setText("Add Label");
        }
        else {
            ui->rcvUpdateLabel->setText("Update Label");
        }

        ui->rcvLabel->setText(label);
        ui->rcvBal->setText(Settings::getHUSHUSDDisplayFormat(rpc->getAllBalances()->value(addr)));
        ui->txtReceive->setPlainText(addr);
        ui->qrcodeDisplay->setQrcodeString(addr);

        if (rpc->getUsedAddresses()->value(addr, false)) {
            ui->rcvBal->setToolTip(tr("Address has been previously used"));
        } else {
            ui->rcvBal->setToolTip(tr("Address is unused"));
        }

    });

    // Receive tab add/update label
    QObject::connect(ui->rcvUpdateLabel, &QPushButton::clicked, [=]() {
        QString addr = ui->listReceiveAddresses->currentText();
        if (addr.isEmpty())
            return;

        auto curLabel = AddressBook::getInstance()->getLabelForAddress(addr);
        auto label = ui->rcvLabel->text().trimmed();

        if (curLabel == label)  // Nothing to update
            return;

        QString info;

        if (!curLabel.isEmpty() && label.isEmpty()) {
            info = "Removed Label '" % curLabel % "'";
            AddressBook::getInstance()->removeAddressLabel(curLabel, addr);
        }
        else if (!curLabel.isEmpty() && !label.isEmpty()) {
            info = "Updated Label '" % curLabel % "' to '" % label % "'";
            AddressBook::getInstance()->updateLabel(curLabel, addr, label);
        }
        else if (curLabel.isEmpty() && !label.isEmpty()) {
            info = "Added Label '" % label % "'";
            AddressBook::getInstance()->addAddressLabel(label, addr);
        }

        // Update labels everywhere on the UI
        updateLabels();

        // Show the user feedback
        if (!info.isEmpty()) {
            QMessageBox::information(this, "Label", info, QMessageBox::Ok);
        }
    });

    // Receive Export Key
    QObject::connect(ui->exportKey, &QPushButton::clicked, [=]() {
        QString addr = ui->listReceiveAddresses->currentText();
        if (addr.isEmpty())
            return;

        this->exportKeys(addr);
    });
}


void MainWindow::updateTAddrCombo(bool checked) {
    if (checked) {
        auto utxos = this->rpc->getUTXOs();
        ui->listReceiveAddresses->clear();

        std::for_each(utxos->begin(), utxos->end(), [=](auto& utxo) {
            auto addr = utxo.address;
            if (addr.startsWith("R") && ui->listReceiveAddresses->findText(addr) < 0) {
                auto bal = rpc->getAllBalances()->value(addr);
                ui->listReceiveAddresses->addItem(addr, bal);
            }
        });
    }
};

// Updates the labels everywhere on the UI. Call this after the labels have been updated
void MainWindow::updateLabels() {
    // Update the Receive tab
    if (ui->rdioTAddr->isChecked()) {
        updateTAddrCombo(true);
    }
    else {
        addZAddrsToComboList(ui->rdioZSAddr->isChecked())(true);
    }

    // Update the Send Tab
    updateFromCombo();

    // Update the autocomplete
    updateLabelsAutoComplete();
}

void MainWindow::slot_change_currency(const QString& currency_name)
{
    qDebug() << __func__ << ": " << currency_name;
    Settings::getInstance()->set_currency_name(currency_name);
    qDebug() << "Refreshing price stats after currency change";
    rpc->refreshPrice();

    // Include currency
    QString saved_currency_name;
    try {
       saved_currency_name = Settings::getInstance()->get_currency_name();
    } catch (const std::exception& e) {
        qDebug() << QString("Ignoring currency change Exception! : ");
        saved_currency_name = "BTC";
    }
}

void MainWindow::slot_change_theme(QString& theme_name)
{
    qDebug() << __func__ << ": theme_name=" << theme_name;

    if (theme_name == "dark" || theme_name == "default" || theme_name == "light" ||
        theme_name == "midnight" || theme_name == "blue" || theme_name == "zdeex") {
        Settings::getInstance()->set_theme_name(theme_name);
    } else {
        qDebug() << __func__ << ": ignoring invalid theme_name=" << theme_name;
        Settings::getInstance()->set_theme_name("dark");
    }

    // Include css
    QString saved_theme_name;
    try {
       saved_theme_name = Settings::getInstance()->get_theme_name();
    } catch (const std::exception& e) {
        qDebug() << QString("Ignoring theme change Exception! : ");

        if(iszdeex){
            saved_theme_name = "zdeex";
        }else{
            saved_theme_name = "dark";
        }
    }

    QString filename = ":/css/" + saved_theme_name +".css";
    QFile qFile(filename);
    qDebug() << __func__ << ": attempting to open filename=" << filename;
    if (qFile.open(QFile::ReadOnly))
    {
      QString styleSheet = QLatin1String(qFile.readAll());
      this->setStyleSheet(""); // reset styles
      this->setStyleSheet(styleSheet);
    }

}

void MainWindow::rescanButtonClicked(int number) {
    qDebug() << __func__ << ": " << number;

    // Setup rescan dialog
    Ui_RescanDialog rescanDialog;
    QDialog dialog(this);
    rescanDialog.setupUi(&dialog);

    rescanDialog.rescanBlockheight->setFocus();
    // Default to full rescan
    rescanDialog.rescanBlockheight->setText("1");

    // Add validator for block height
    QRegExpValidator* heightValidator = new QRegExpValidator(QRegExp("\\d*"), this);
    rescanDialog.rescanBlockheight->setValidator(heightValidator);

    // Check if OK clicked
    if (dialog.exec() == QDialog::Accepted) {
        // Get submitted rescan height
        int rescanHeight = rescanDialog.rescanBlockheight->text().toInt();
        qDebug() << __func__ << ": rescan height = " << rescanHeight;

        // Show message in status bar
        ui->statusBar->showMessage(tr("Rescanning...") + tr(" from height ") + QString::number(rescanHeight), 5000);

        // Close settings
        QWidget *modalWidget = QApplication::activeModalWidget();
        if (modalWidget)
            modalWidget->close();


        // Call rescan RPC
        // TODO: This RPC might timeout, does the callback work correctly in that case?
        rpc->rescan(rescanHeight, [=] (QJsonValue response){
            qDebug() << __func__ << ":rescanning finished" << response;
            ui->statusBar->showMessage(tr("Rescanning finished"), 5000);
        });

        qDebug() << __func__ << ": force refresh of rescan data";
        rpc->refreshRescan();

    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete rpc;
    delete labelCompleter;

    delete amtValidator;
    delete feesValidator;

    delete loadingMovie;
    delete logger;

}
