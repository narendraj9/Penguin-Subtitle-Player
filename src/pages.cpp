#include "pages.h"
#include "QLineEdit"
#include "configdialog.h"
#include "prefconstants.h"
#include "prefpage.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

void GeneralPage::load() {
  dirEdit->setPlainText(settings.value("gen/dir").toString());
  useDetectedEncodingCbx->setChecked(
      settings
          .value("gen/useDetectedEncoding",
                 QVariant::fromValue(PrefConstants::USE_DETECTED_ENCODING))
          .toBool());
  adjustIntervalSpinBox->setValue(
      settings
          .value("gen/adjust",
                 QVariant::fromValue(PrefConstants::ADJUST_INTERVAL))
          .toInt());

  speedFactorSpinBox->setValue(
      settings
          .value("gen/speedFactor",
                 QVariant::fromValue(PrefConstants::SPEED_FACTOR))
          .toDouble());

  bool isResetSpeedFactorOnLaunch =
      settings
          .value(
              "gen/resetSpeedFactorOnLaunch",
              QVariant::fromValue(PrefConstants::RESET_SPEED_FACTOR_ON_LAUNCH))
          .toBool();
  resetSpeedFactorOnLaunchCbx->setChecked(isResetSpeedFactorOnLaunch);
}

void GeneralPage::save() {
  settings.setValue("gen/dir", dirEdit->toPlainText());
  settings.setValue("gen/useDetectedEncoding",
                    useDetectedEncodingCbx->isChecked());
  settings.setValue("gen/adjust", adjustIntervalSpinBox->value());
  settings.setValue("gen/speedFactor", speedFactorSpinBox->value());
  settings.setValue("gen/resetSpeedFactorOnLaunch",
                    resetSpeedFactorOnLaunchCbx->isChecked());
}

void GeneralPage::openDirDialog() {
  QString path = QFileDialog::getExistingDirectory();
  if (!path.isNull())
    dirEdit->setPlainText(path);
}

void GeneralPage::resetSettings() {
  settings.clear();
  this->configDialog->load();
}

GeneralPage::GeneralPage(QWidget *parent, ConfigDialog *configDialog)
    : PrefPage(parent, configDialog) {
  QGroupBox *filesGroup = new QGroupBox(tr("Files"));

  QGroupBox *encodingGroup = new QGroupBox(tr("Encoding"));

  QGroupBox *adjustGroup = new QGroupBox(tr("Adjustment"));

  QGroupBox *speedGroup = new QGroupBox(tr("Speed"));

  QLabel *defaultDirLabel = new QLabel(tr("Default directory:"));
  dirEdit = new QPlainTextEdit();
  QPushButton *dirBrowseButton = new QPushButton(tr("Browse"));
  connect(dirBrowseButton, SIGNAL(clicked()), this, SLOT(openDirDialog()));

  useDetectedEncodingCbx =
      new QCheckBox(tr("Use detected encoding without prompt"));

  QLabel *adjustIntervalLabel =
      new QLabel(tr("Time adjustment interval (ms): "));
  adjustIntervalSpinBox = new QSpinBox();
  adjustIntervalSpinBox->setSingleStep(PrefConstants::ADJUST_INTERVAL_STEP);
  adjustIntervalSpinBox->setMaximum(PrefConstants::ADJUST_INTERVAL_MAX);

  QLabel *speedFactorLabel = new QLabel(tr("Speed factor : "));
  speedFactorSpinBox = new QDoubleSpinBox();
  speedFactorSpinBox->setDecimals(3);
  speedFactorSpinBox->setSingleStep(PrefConstants::SPEED_FACTOR_STEP);
  speedFactorSpinBox->setMaximum(PrefConstants::SPEED_FACTOR_MAX);
  speedFactorSpinBox->setMinimum(PrefConstants::SPEED_FACTOR_MIN);
  resetSpeedFactorOnLaunchCbx = new QCheckBox(tr("Resets to 1.00 on launch"));
  QSpacerItem *spacerItem =
      new QSpacerItem(10, 1, QSizePolicy::Preferred, QSizePolicy::Preferred);

  QHBoxLayout *defaultDirLayout = new QHBoxLayout;
  defaultDirLayout->addWidget(defaultDirLabel);
  defaultDirLayout->addWidget(dirEdit);
  defaultDirLayout->addWidget(dirBrowseButton);

  QHBoxLayout *adjustIntervalLayout = new QHBoxLayout;
  adjustIntervalLayout->addWidget(adjustIntervalLabel);
  adjustIntervalLayout->addWidget(adjustIntervalSpinBox);
  adjustIntervalLayout->addStretch(1);

  QHBoxLayout *speedFactorLayout = new QHBoxLayout;
  speedFactorLayout->addWidget(speedFactorLabel);
  speedFactorLayout->addWidget(speedFactorSpinBox);
  speedFactorLayout->addItem(spacerItem);
  speedFactorLayout->addWidget(resetSpeedFactorOnLaunchCbx);
  speedFactorLayout->addStretch(1);

  QPushButton *resetButton = new QPushButton(tr("Reset all preferences"));
  connect(resetButton, SIGNAL(clicked()), this, SLOT(resetSettings()));

  QVBoxLayout *configLayout = new QVBoxLayout;
  configLayout->addLayout(defaultDirLayout);
  filesGroup->setLayout(configLayout);

  QVBoxLayout *encodingLayout = new QVBoxLayout;
  encodingLayout->addWidget(useDetectedEncodingCbx);
  encodingGroup->setLayout(encodingLayout);

  QVBoxLayout *adjustLayout = new QVBoxLayout;
  adjustLayout->addLayout(adjustIntervalLayout);
  adjustGroup->setLayout(adjustLayout);

  QVBoxLayout *speedLayout = new QVBoxLayout;
  speedLayout->addLayout(speedFactorLayout);
  speedGroup->setLayout(speedLayout);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(filesGroup);
  mainLayout->addWidget(encodingGroup);
  mainLayout->addWidget(adjustGroup);
  mainLayout->addWidget(speedGroup);
  mainLayout->addWidget(resetButton);
  mainLayout->addStretch(1);
  setLayout(mainLayout);

  this->load();
}

QColor AppearancePage::openColorDialog(QColor initial) {
  QColor color = QColorDialog::getColor(initial, 0, tr("Select color"));
  if (color.isValid()) {
    return color;
  }
  return initial;
}

void AppearancePage::openBgColorDialog() {
  bgColor = openColorDialog(bgColor);
  paintColorButton(bgColorButton, bgColor);
}

void AppearancePage::openFontColorDialog() {
  fontColor = openColorDialog(fontColor);
  paintColorButton(fontColorButton, fontColor);
}

void AppearancePage::openFontShadowColorDialog() {
  fontShadowColor = openColorDialog(fontShadowColor);
  paintColorButton(fontShadowColorButton, fontShadowColor);
}

void AppearancePage::load() {
  /* Window */
  bool isRememberWindowPosAndSize =
      settings
          .value(
              "appearance/rememberWindowPosAndSize",
              QVariant::fromValue(PrefConstants::REMEMBER_WINDOW_POS_AND_SIZE))
          .toBool();
  rememberWindowPosAndSizeCbx->setChecked(isRememberWindowPosAndSize);
  bgColor =
      QColor::fromRgb(settings
                          .value("appearance/bgColor",
                                 QVariant::fromValue(PrefConstants::BG_COLOR))
                          .toUInt());
  paintColorButton(bgColorButton, bgColor);
  bgAlphaSlider->setValue(
      settings
          .value("appearance/bgAlpha",
                 QVariant::fromValue(PrefConstants::BG_ALPHA))
          .toInt());

  /* Font */
  fontColor =
      QColor::fromRgb(settings
                          .value("appearance/fontColor",
                                 QVariant::fromValue(PrefConstants::FONT_COLOR))
                          .toUInt());
  paintColorButton(fontColorButton, fontColor);

  QFont initial;
  initial.fromString(
      settings.value("appearance/font", PrefConstants::FONT).toString());
  fontDialog->setCurrentFont(initial);

  /* Font Shadow */
  bool isFontShadowEnable =
      settings
          .value("appearance/fontShadowEnable",
                 QVariant::fromValue(PrefConstants::FONT_SHADOW_ENABLE))
          .toBool();
  fontShadowEnableCbx->setChecked(isFontShadowEnable);
  fontShadowColor = QColor::fromRgb(
      settings
          .value("appearance/fontShadowColor",
                 QVariant::fromValue(PrefConstants::FONT_SHADOW_COLOR))
          .toUInt());
  paintColorButton(fontShadowColorButton, fontShadowColor);
  fontShadowBlurRadiusSpinBox->setValue(
      settings
          .value("appearance/fontShadowBlurRadius",
                 QVariant::fromValue(PrefConstants::FONT_SHADOW_BLUR_RADIUS))
          .toInt());
  fontShadowOffsetXSpinBox->setValue(
      settings
          .value("appearance/fontShadowOffsetX",
                 QVariant::fromValue(PrefConstants::FONT_SHADOW_OFFSET_X))
          .toInt());
  fontShadowOffsetYSpinBox->setValue(
      settings
          .value("appearance/fontShadowOffsetY",
                 QVariant::fromValue(PrefConstants::FONT_SHADOW_OFFSET_Y))
          .toInt());
}

void AppearancePage::save() {
  /* Window */
  settings.setValue("appearance/rememberWindowPosAndSize",
                    rememberWindowPosAndSizeCbx->isChecked());
  settings.setValue("appearance/bgColor", bgColor.rgb());
  settings.setValue("appearance/bgAlpha", bgAlphaSlider->value());

  /* Font */
  settings.setValue("appearance/fontColor", fontColor.rgb());
  settings.setValue("appearance/font", fontDialog->currentFont().toString());

  /* Font Shadow */
  settings.setValue("appearance/fontShadowEnable",
                    fontShadowEnableCbx->isChecked());
  settings.setValue("appearance/fontShadowColor", fontShadowColor.rgb());
  settings.setValue("appearance/fontShadowBlurRadius",
                    fontShadowBlurRadiusSpinBox->value());
  settings.setValue("appearance/fontShadowOffsetX",
                    fontShadowOffsetXSpinBox->value());
  settings.setValue("appearance/fontShadowOffsetY",
                    fontShadowOffsetYSpinBox->value());
}

AppearancePage::~AppearancePage() {}

AppearancePage::AppearancePage(QWidget *parent, ConfigDialog *configDialog)
    : PrefPage(parent, configDialog) {

  /* Window */
  QGroupBox *windowAppearanceGroup = new QGroupBox(tr("Window"));

  rememberWindowPosAndSizeCbx =
      new QCheckBox(tr("Remember last position and size"));

  QLabel *bgColorLabel = new QLabel(tr("Background color: "));
  bgColorButton = new QPushButton();

  connect(bgColorButton, SIGNAL(clicked()), this, SLOT(openBgColorDialog()));

  QHBoxLayout *bgColorLayout = new QHBoxLayout;
  bgColorLayout->addWidget(bgColorLabel);
  bgColorLayout->addWidget(bgColorButton);
  bgColorLayout->addStretch(1);

  QLabel *bgAlphaLabel = new QLabel(tr("Opacity: "));
  bgAlphaSlider = new QSlider(Qt::Horizontal);
  bgAlphaSlider->setRange(PrefConstants::BG_ALPHA_MIN, 255);

  QHBoxLayout *bgAlphaLayout = new QHBoxLayout;
  bgAlphaLayout->addWidget(bgAlphaLabel);
  bgAlphaLayout->addWidget(bgAlphaSlider);

  QVBoxLayout *windowAppearanceLayout = new QVBoxLayout;
  windowAppearanceLayout->addWidget(rememberWindowPosAndSizeCbx);
  windowAppearanceLayout->addLayout(bgColorLayout);
  windowAppearanceLayout->addLayout(bgAlphaLayout);
  windowAppearanceGroup->setLayout(windowAppearanceLayout);

  /* Subtitle Font */
  QGroupBox *fontGroup = new QGroupBox(tr("Subtitle Font"));

  QLabel *fontColorLabel = new QLabel(tr("Font color: "));
  fontColorButton = new QPushButton();
  connect(fontColorButton, SIGNAL(clicked()), this,
          SLOT(openFontColorDialog()));

  QHBoxLayout *fontColorLayout = new QHBoxLayout;
  fontColorLayout->addWidget(fontColorLabel);
  fontColorLayout->addWidget(fontColorButton);
  fontColorLayout->addStretch(1);

  fontDialog = new QFontDialog();
  fontDialog->setWindowFlags(Qt::Widget);
  fontDialog->setOptions(QFontDialog::NoButtons |
                         QFontDialog::DontUseNativeDialog);

  QVBoxLayout *fontLayout = new QVBoxLayout;
  fontLayout->addLayout(fontColorLayout);
  fontLayout->addWidget(fontDialog);
  fontGroup->setLayout(fontLayout);

  /* Subtitle Font Drop Shadow */
  QGroupBox *fontShadowGroup = new QGroupBox(tr("Subtitle Text Outline"));

  /* Enable */
  fontShadowEnableCbx = new QCheckBox(tr("Enable shadow"));

  QHBoxLayout *fontShadowEnableLayout = new QHBoxLayout;
  fontShadowEnableLayout->addWidget(fontShadowEnableCbx);
  fontShadowEnableLayout->addStretch(1);

  /* Color */
  QLabel *fontShadowColorLabel = new QLabel(tr("Shadow color: "));
  fontShadowColorButton = new QPushButton();
  connect(fontShadowColorButton, SIGNAL(clicked()), this,
          SLOT(openFontShadowColorDialog()));

  QHBoxLayout *fontShadowColorLayout = new QHBoxLayout;
  fontShadowColorLayout->addWidget(fontShadowColorLabel);
  fontShadowColorLayout->addWidget(fontShadowColorButton);
  fontShadowColorLayout->addStretch(1);

  /* Blur Radius */
  QLabel *fontShadowBlurRadiusLabel = new QLabel(tr("Blur radius: "));
  fontShadowBlurRadiusSpinBox = new QSpinBox();
  fontShadowBlurRadiusSpinBox->setSingleStep(
      PrefConstants::FONT_SHADOW_BLUR_RADIUS_STEP);
  fontShadowBlurRadiusSpinBox->setMaximum(
      PrefConstants::FONT_SHADOW_BLUR_RADIUS_MAX);

  QHBoxLayout *fontShadowBlurRadiusLayout = new QHBoxLayout;
  fontShadowBlurRadiusLayout->addWidget(fontShadowBlurRadiusLabel);
  fontShadowBlurRadiusLayout->addWidget(fontShadowBlurRadiusSpinBox);
  fontShadowBlurRadiusLayout->addStretch(1);

  /* Offset */
  QLabel *fontShadowOffsetLabel = new QLabel(tr("Offset X, Y: "));
  fontShadowOffsetXSpinBox = new QSpinBox();
  fontShadowOffsetXSpinBox->setMinimum(
      -PrefConstants::FONT_SHADOW_OFFSET_LIMIT);
  fontShadowOffsetXSpinBox->setMaximum(PrefConstants::FONT_SHADOW_OFFSET_LIMIT);
  fontShadowOffsetYSpinBox = new QSpinBox();
  fontShadowOffsetYSpinBox->setMinimum(
      -PrefConstants::FONT_SHADOW_OFFSET_LIMIT);
  fontShadowOffsetYSpinBox->setMaximum(PrefConstants::FONT_SHADOW_OFFSET_LIMIT);

  QHBoxLayout *fontShadowOffsetLayout = new QHBoxLayout;
  fontShadowOffsetLayout->addWidget(fontShadowOffsetLabel);
  fontShadowOffsetLayout->addWidget(fontShadowOffsetXSpinBox);
  fontShadowOffsetLayout->addWidget(fontShadowOffsetYSpinBox);
  fontShadowOffsetLayout->addStretch(1);

  QVBoxLayout *fontShadowLayout = new QVBoxLayout;
  fontShadowLayout->addLayout(fontShadowEnableLayout);
  fontShadowLayout->addLayout(fontShadowColorLayout);
  fontShadowLayout->addLayout(fontShadowBlurRadiusLayout);
  fontShadowLayout->addLayout(fontShadowOffsetLayout);
  fontShadowGroup->setLayout(fontShadowLayout);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(windowAppearanceGroup);
  mainLayout->addWidget(fontGroup);
  mainLayout->addWidget(fontShadowGroup);
  mainLayout->addSpacing(12);
  mainLayout->addStretch(1);
  setLayout(mainLayout);

  this->load();
}

void AppearancePage::paintColorButton(QPushButton *button, QColor color) {
  QPixmap px(64, 64);
  QPainter pt(&px);
  pt.setBrush(color);
  pt.drawRect(0, 0, px.width() - 1, px.height() - 1);
  button->setIcon(color.isValid() ? px : QIcon());
}

AboutPage::AboutPage(QWidget *parent, ConfigDialog *configDialog)
    : PrefPage(parent, configDialog) {

  QLabel *nameLabel = new QLabel(tr("Penguin Subtitle Player"));
  QFont font = nameLabel->font();
  font.setPointSize(20);
  font.setBold(true);
  nameLabel->setFont(font);
  QPixmap iconPixmap(":/icon.png");
  iconPixmap = iconPixmap.scaled(QSize(50, 50), Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
  QLabel *iconLabel = new QLabel();
  iconLabel->setPixmap(iconPixmap);

  QHBoxLayout *nameLayout = new QHBoxLayout;
  nameLayout->addWidget(iconLabel);
  nameLayout->addWidget(nameLabel);
  nameLayout->addStretch(1);

  QLabel *versionLabel = new QLabel(tr("Version: "));
  QLabel *versionValueLabel =
      new QLabel(QString(QCoreApplication::applicationVersion()));

  QHBoxLayout *versionLayout = new QHBoxLayout;
  versionLayout->addWidget(versionLabel);
  versionLayout->addWidget(versionValueLabel);
  versionLayout->addStretch(1);

  QLabel *websiteLabel = new QLabel(tr("Website: "));
  QLabel *websiteValueLabel =
      new QLabel("<a "
                 "href=\"https://github.com/carsonip/"
                 "Penguin-Subtitle-Player\">github.com/carsonip/"
                 "Penguin-Subtitle-Player</a>");
  websiteValueLabel->setTextFormat(Qt::RichText);
  websiteValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  websiteValueLabel->setOpenExternalLinks(true);

  QHBoxLayout *websiteLayout = new QHBoxLayout;
  websiteLayout->addWidget(websiteLabel);
  websiteLayout->addWidget(websiteValueLabel);
  websiteLayout->addStretch(1);

  QLabel *issuesLabel = new QLabel(tr("Issue tracker: "));
  QLabel *issuesValueLabel = new QLabel(
      "<a "
      "href=\"https://github.com/carsonip/Penguin-Subtitle-Player/"
      "issues\">github.com/carsonip/Penguin-Subtitle-Player/issues</a>");
  issuesValueLabel->setTextFormat(Qt::RichText);
  issuesValueLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  issuesValueLabel->setOpenExternalLinks(true);

  QHBoxLayout *issuesLayout = new QHBoxLayout;
  issuesLayout->addWidget(issuesLabel);
  issuesLayout->addWidget(issuesValueLabel);
  issuesLayout->addStretch(1);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(nameLayout);
  mainLayout->addLayout(versionLayout);
  mainLayout->addLayout(websiteLayout);
  mainLayout->addLayout(issuesLayout);
  mainLayout->addStretch(1);

  setLayout(mainLayout);
}

AboutPage::~AboutPage() {}

void AboutPage::load() {}
void AboutPage::save() {}

// ---------- LearningPage ----------

LearningPage::LearningPage(QWidget *parent, ConfigDialog *configDialog)
    : PrefPage(parent, configDialog) {

    QGroupBox *modeGroup = new QGroupBox(tr("Learning Mode"));

    learningModeCbx = new QCheckBox(tr("Enable learning mode (hide translation until T is pressed)"));

    QLabel *thinkTimeLabel = new QLabel(tr("Think time before hint (seconds, 0 = immediate):"));
    thinkTimeSpinBox = new QSpinBox();
    thinkTimeSpinBox->setRange(0, 60);
    thinkTimeSpinBox->setSuffix(tr(" s"));

    QLabel *autoHideLabel = new QLabel(tr("Auto-hide translation after reveal (seconds, 0 = stay visible):"));
    autoHideDelaySpinBox = new QSpinBox();
    autoHideDelaySpinBox->setRange(0, 120);
    autoHideDelaySpinBox->setSuffix(tr(" s"));

    QVBoxLayout *modeLayout = new QVBoxLayout();
    modeLayout->addWidget(learningModeCbx);
    QHBoxLayout *thinkRow = new QHBoxLayout();
    thinkRow->addWidget(thinkTimeLabel);
    thinkRow->addWidget(thinkTimeSpinBox);
    thinkRow->addStretch(1);
    modeLayout->addLayout(thinkRow);
    QHBoxLayout *hideRow = new QHBoxLayout();
    hideRow->addWidget(autoHideLabel);
    hideRow->addWidget(autoHideDelaySpinBox);
    hideRow->addStretch(1);
    modeLayout->addLayout(hideRow);
    modeGroup->setLayout(modeLayout);

    QGroupBox *vocabGroup = new QGroupBox(tr("Vocabulary Highlights"));

    inlineHighlightsCbx = new QCheckBox(tr("Show inline word highlights and colour legend"));

    QLabel *legendFontSizeLabel = new QLabel(tr("Word meaning text size:"));
    legendFontSizeSpinBox = new QSpinBox();
    legendFontSizeSpinBox->setRange(8, 72);
    legendFontSizeSpinBox->setSuffix(tr(" px"));

    QLabel *legendBgColorLabel = new QLabel(tr("Word meaning background:"));
    legendBgColorButton = new QPushButton();
    legendBgColorButton->setFixedSize(42, 24);
    connect(legendBgColorButton, SIGNAL(clicked()), this,
            SLOT(openLegendBgColorDialog()));

    QLabel *legendBgAlphaLabel = new QLabel(tr("Background opacity:"));
    legendBgAlphaSlider = new QSlider(Qt::Horizontal);
    legendBgAlphaSlider->setRange(0, 255);
    legendBgAlphaSlider->setFixedWidth(140);

    QLabel *vocabFileLabel = new QLabel(tr("Vocabulary file (vocab.json, optional):"));
    vocabFileEdit = new QLineEdit();
    vocabFileEdit->setPlaceholderText(tr("Path to vocab.json …"));
    QPushButton *browseBtn = new QPushButton(tr("Browse"));
    connect(browseBtn, SIGNAL(clicked()), this, SLOT(browseVocabFile()));

    QHBoxLayout *fileRow = new QHBoxLayout();
    fileRow->addWidget(vocabFileEdit, 1);
    fileRow->addWidget(browseBtn);

    QHBoxLayout *legendSizeRow = new QHBoxLayout();
    legendSizeRow->addWidget(legendFontSizeLabel);
    legendSizeRow->addWidget(legendFontSizeSpinBox);
    legendSizeRow->addStretch(1);

    QHBoxLayout *legendBgRow = new QHBoxLayout();
    legendBgRow->addWidget(legendBgColorLabel);
    legendBgRow->addWidget(legendBgColorButton);
    legendBgRow->addSpacing(12);
    legendBgRow->addWidget(legendBgAlphaLabel);
    legendBgRow->addWidget(legendBgAlphaSlider);
    legendBgRow->addStretch(1);

    QVBoxLayout *vocabLayout = new QVBoxLayout();
    vocabLayout->addWidget(inlineHighlightsCbx);
    vocabLayout->addLayout(legendSizeRow);
    vocabLayout->addLayout(legendBgRow);
    vocabLayout->addWidget(vocabFileLabel);
    vocabLayout->addLayout(fileRow);
    vocabGroup->setLayout(vocabLayout);

    QGroupBox *apiGroup = new QGroupBox(tr("Vocabulary Extraction API (optional)"));

    QLabel *providerLabel = new QLabel(tr("Provider:"));
    apiProviderCombo = new QComboBox();
    apiProviderCombo->addItem("OpenAI (gpt-4.1) — best quality");
    apiProviderCombo->addItem("Groq (gpt-oss-120b) — biggest Groq model");
    apiProviderCombo->addItem("Cerebras (gpt-oss-120b) — fast/free");

    QLabel *keyLabel = new QLabel(tr("API key:"));
    apiKeyEdit = new QLineEdit();
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiKeyEdit->setPlaceholderText(tr("Paste your API key here"));

    QLabel *hintLabel = new QLabel(
        tr("With an API key configured, Penguin can auto-highlight\n"
           "B1-level German words and show meanings for each subtitle.\n"
           "Best quality: OpenAI gpt-4.1. Free/fast: Groq or Cerebras."));
    hintLabel->setStyleSheet("color: #777777; font-size: 11px;");

    QVBoxLayout *apiLayout = new QVBoxLayout();
    QHBoxLayout *provRow = new QHBoxLayout();
    provRow->addWidget(providerLabel);
    provRow->addWidget(apiProviderCombo);
    provRow->addStretch(1);
    apiLayout->addLayout(provRow);
    QHBoxLayout *keyRow = new QHBoxLayout();
    keyRow->addWidget(keyLabel);
    keyRow->addWidget(apiKeyEdit, 1);
    apiLayout->addLayout(keyRow);
    apiLayout->addWidget(hintLabel);
    apiGroup->setLayout(apiLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(modeGroup);
    mainLayout->addWidget(vocabGroup);
    mainLayout->addWidget(apiGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    this->load();
}

void LearningPage::paintColorButton(QPushButton *button, QColor color) {
    QPixmap px(16, 16);
    px.fill(Qt::transparent);
    QPainter pt(&px);
    pt.setPen(Qt::black);
    pt.setBrush(color);
    pt.drawRect(0, 0, 15, 15);
    button->setIcon(QIcon(px));
}

void LearningPage::load() {
    learningModeCbx->setChecked(
        settings.value("learning/enabled",
                       QVariant::fromValue(PrefConstants::LEARNING_MODE_ENABLED))
            .toBool());
    thinkTimeSpinBox->setValue(
        settings.value("learning/thinkTime",
                       QVariant::fromValue(PrefConstants::LEARNING_THINK_TIME))
            .toInt());
    autoHideDelaySpinBox->setValue(
        settings
            .value("learning/autoHideDelay",
                   QVariant::fromValue(PrefConstants::LEARNING_AUTO_HIDE_DELAY))
            .toInt());
    inlineHighlightsCbx->setChecked(
        settings
            .value("learning/inlineHighlights",
                   QVariant::fromValue(PrefConstants::INLINE_HIGHLIGHTS_ENABLED))
            .toBool());
    legendFontSizeSpinBox->setValue(
        settings
            .value("learning/legendFontSize",
                   QVariant::fromValue(PrefConstants::INLINE_LEGEND_FONT_SIZE))
            .toInt());
    legendBgColor = QColor::fromRgb(
        settings
            .value("learning/legendBgColor",
                   QVariant::fromValue(PrefConstants::INLINE_LEGEND_BG_COLOR))
            .toUInt());
    paintColorButton(legendBgColorButton, legendBgColor);
    legendBgAlphaSlider->setValue(
        settings
            .value("learning/legendBgAlpha",
                   QVariant::fromValue(PrefConstants::INLINE_LEGEND_BG_ALPHA))
            .toInt());
    vocabFileEdit->setText(settings.value("learning/vocabFile").toString());
    apiProviderCombo->setCurrentIndex(
        settings.value("learning/apiProvider", 0).toInt());
    apiKeyEdit->setText(settings.value("learning/apiKey").toString());
}

void LearningPage::save() {
    settings.setValue("learning/enabled", learningModeCbx->isChecked());
    settings.setValue("learning/thinkTime", thinkTimeSpinBox->value());
    settings.setValue("learning/autoHideDelay", autoHideDelaySpinBox->value());
    settings.setValue("learning/inlineHighlights", inlineHighlightsCbx->isChecked());
    settings.setValue("learning/legendFontSize", legendFontSizeSpinBox->value());
    settings.setValue("learning/legendBgColor", legendBgColor.rgb());
    settings.setValue("learning/legendBgAlpha", legendBgAlphaSlider->value());
    settings.setValue("learning/vocabFile", vocabFileEdit->text());
    settings.setValue("learning/apiProvider", apiProviderCombo->currentIndex());
    settings.setValue("learning/apiKey", apiKeyEdit->text());
}

void LearningPage::openLegendBgColorDialog() {
    QColor color = QColorDialog::getColor(legendBgColor, this,
                                          tr("Select word meaning background"));
    if (color.isValid()) {
        legendBgColor = color;
        paintColorButton(legendBgColorButton, legendBgColor);
    }
}

void LearningPage::browseVocabFile() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select Vocabulary File"), "",
        tr("JSON files (*.json);;All files (*)"));
    if (!path.isEmpty())
        vocabFileEdit->setText(path);
}
