#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

static QLineEdit* addRow(QFormLayout* layout, const QString& label, const QString& value) {
    auto* l = new QLabel(label);
    auto* e = new QLineEdit(value);
    layout->addRow(l, e);
    return e;
}

static QFrame* makeCard(const QString& title, const QString& subtitle, QWidget* content) {
    auto* card = new QFrame();
    card->setObjectName("card");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    if (!title.isEmpty()) {
        auto* titleLabel = new QLabel(title);
        titleLabel->setObjectName("cardTitle");
        layout->addWidget(titleLabel);
    }

    if (!subtitle.isEmpty()) {
        auto* subtitleLabel = new QLabel(subtitle);
        subtitleLabel->setObjectName("cardSubtitle");
        subtitleLabel->setWordWrap(true);
        layout->addWidget(subtitleLabel);
    }

    layout->addWidget(content);
    return card;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    app.setStyle("Fusion");
    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    baseFont.setPointSize(10);
    app.setFont(baseFont);

    QWidget window;
    window.setObjectName("window");
    window.setWindowTitle("大气透过率计算器");

    app.setStyleSheet(
        "#window {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0f2027, stop:0.5 #203a43, stop:1 #2c5364);"
        "}"
        "QLabel { color: #e6eef4; }"
        "QLineEdit {"
        "  background: #0f1b21; color: #e6eef4;"
        "  border: 1px solid #2d4651; border-radius: 6px; padding: 6px 8px;"
        "}"
        "QLineEdit:focus { border: 1px solid #49c1ff; }"
        "#card {"
        "  background: #101c23;"
        "  border: 1px solid #1f313b; border-radius: 12px;"
        "}"
        "#cardTitle { font-size: 16px; font-weight: 600; background: transparent; }"
        "#cardSubtitle { color: #a9bdc9; background: transparent; }"
        "QGroupBox {"
        "  border: 1px solid #2d4651; border-radius: 10px; margin-top: 10px;"
        "  color: #e6eef4;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 6px;"
        "}"
        "QPushButton {"
        "  background: #1f313b; color: #e6eef4;"
        "  border: 1px solid #35515f; border-radius: 8px; padding: 8px 14px;"
        "}"
        "QPushButton:hover { background: #27404c; }"
        "QPushButton:pressed { background: #1a2b33; }"
        "#primaryButton {"
        "  background: #49c1ff; color: #0b1216; border: none; font-weight: 600;"
        "}"
        "#primaryButton:hover { background: #62c9ff; }"
        "#primaryButton:pressed { background: #2aa9ea; }"
        "QTextEdit {"
        "  background: #0f1b21; color: #e6eef4;"
        "  border: 1px solid #2d4651; border-radius: 8px; padding: 8px;"
        "}"
        "QScrollArea, QScrollArea > QWidget > QWidget {"
        "  background: transparent;"
        "}"
    );

    auto* mainLayout = new QHBoxLayout(&window);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    auto* leftWidget = new QWidget();
    auto* root = new QVBoxLayout(leftWidget);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(24, 24, 24, 24);

    auto* chartTitle = new QLabel("结果图表");
    chartTitle->setObjectName("cardTitle");
    chartTitle->setStyleSheet("font-size: 20px; font-weight: 700;");
    rightLayout->addWidget(chartTitle);

    auto* chartTrans = new QChart();
    chartTrans->setTitle("透过率曲线 (Transmittance)");
    chartTrans->setTheme(QChart::ChartThemeDark);
    chartTrans->setBackgroundBrush(QColor("#101c23"));
    chartTrans->legend()->hide();

    auto* viewTrans = new QChartView(chartTrans);
    viewTrans->setRenderHint(QPainter::Antialiasing);
    viewTrans->setStyleSheet("background: transparent; border: 1px solid #1f313b; border-radius: 12px;");
    
    auto* chartH2O = new QChart();
    chartH2O->setTitle("H2O 吸收系数");
    chartH2O->setTheme(QChart::ChartThemeDark);
    chartH2O->setBackgroundBrush(QColor("#101c23"));
    chartH2O->legend()->hide();

    auto* viewH2O = new QChartView(chartH2O);
    viewH2O->setRenderHint(QPainter::Antialiasing);
    viewH2O->setStyleSheet("background: transparent; border: 1px solid #1f313b; border-radius: 12px;");

    auto* chartCO2 = new QChart();
    chartCO2->setTitle("CO2 吸收系数");
    chartCO2->setTheme(QChart::ChartThemeDark);
    chartCO2->setBackgroundBrush(QColor("#101c23"));
    chartCO2->legend()->hide();

    auto* viewCO2 = new QChartView(chartCO2);
    viewCO2->setRenderHint(QPainter::Antialiasing);
    viewCO2->setStyleSheet("background: transparent; border: 1px solid #1f313b; border-radius: 12px;");

    auto* splitterRight = new QSplitter(Qt::Vertical);
    splitterRight->addWidget(viewTrans);
    splitterRight->addWidget(viewH2O);
    splitterRight->addWidget(viewCO2);

    rightLayout->addWidget(splitterRight);

    auto* transLabel = new QLabel("平均透过率: --");
    transLabel->setObjectName("cardTitle");
    transLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #49c1ff; margin-top: 10px;");
    transLabel->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(transLabel);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    auto* header = new QWidget();
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);
    auto* title = new QLabel("大气吸收透过率计算");
    title->setObjectName("cardTitle");
    title->setStyleSheet("font-size: 22px; font-weight: 700;");
    auto* subtitle = new QLabel("快速填写参数，生成大气透过率计算结果");
    subtitle->setObjectName("cardSubtitle");
    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    root->addWidget(header);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    root->addWidget(scroll);

    auto* content = new QWidget();
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);
    scroll->setWidget(content);

    QString defaultExePath = QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath("../../src/atm.exe"));

    auto* basicFormWidget = new QWidget();
    auto* basicForm = new QFormLayout(basicFormWidget);
    basicForm->setContentsMargins(0, 0, 0, 0);
    basicForm->setSpacing(10);
    basicForm->setLabelAlignment(Qt::AlignLeft);
    basicForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto* rh = addRow(basicForm, "相对湿度 RH (0-1)", "0.46");
    auto* tempC = addRow(basicForm, "温度 (C)", "15");
    auto* ground = addRow(basicForm, "水平距离 GroundRange_km", "1.0");
    auto* h1 = addRow(basicForm, "起始高度 H1_km", "0.0");
    auto* h2 = addRow(basicForm, "终止高度 H2_km", "0.0");

    rh->setPlaceholderText("0.0 - 1.0");
    tempC->setPlaceholderText("15");
    ground->setPlaceholderText("1.0");
    h1->setPlaceholderText("0.0");
    h2->setPlaceholderText("0.0");

    contentLayout->addWidget(makeCard("基础参数", "用于快速计算标准场景", basicFormWidget));

    auto* advancedBox = new QGroupBox("高级选项");
    advancedBox->setObjectName("advancedBox");
    advancedBox->setCheckable(true);
    advancedBox->setChecked(false);
    auto* advContainer = new QWidget();
    auto* advLayout = new QFormLayout(advContainer);
    advLayout->setContentsMargins(0, 0, 0, 0);
    advLayout->setSpacing(10);

    auto* exePath = addRow(advLayout, "atm.exe 路径", defaultExePath);
    auto* f0 = addRow(advLayout, "f0 (g/m^3)", "6.8");
    auto* co2ppm = addRow(advLayout, "CO2 ppm", "400");
    auto* scaleByMix = addRow(advLayout, "按混合比缩放 (1/0)", "1");
    auto* startUm = addRow(advLayout, "start_um", "3");
    auto* endUm = addRow(advLayout, "end_um", "5");
    auto* stepUm = addRow(advLayout, "step_um", "0.05");
    auto* h2oScale = addRow(advLayout, "H2O_scale (-1默认)", "-1");
    auto* co2Scale = addRow(advLayout, "CO2_scale (-1默认)", "-1");
    auto* h2oAltScale = addRow(advLayout, "H2O_alt_scale (-1默认)", "-1");
    auto* h2oCsv = addRow(advLayout, "H2O CSV 路径", "-");
    auto* co2Csv = addRow(advLayout, "CO2 CSV 路径", "-");

    auto* advBoxLayout = new QVBoxLayout(advancedBox);
    advBoxLayout->addWidget(advContainer);
    advContainer->setVisible(false);
    QObject::connect(advancedBox, &QGroupBox::toggled, advContainer, &QWidget::setVisible);

    contentLayout->addWidget(makeCard("高级参数", "需要完整控制时再展开", advancedBox));

    auto* actions = new QWidget();
    auto* actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(12);
    auto* status = new QLabel("准备就绪");
    status->setObjectName("cardSubtitle");
    status->setStyleSheet("background: transparent;");
    auto* runBtn = new QPushButton("运行");
    runBtn->setObjectName("primaryButton");
    runBtn->setMinimumHeight(36);
    actionsLayout->addWidget(status);
    actionsLayout->addStretch();
    actionsLayout->addWidget(runBtn);
    contentLayout->addWidget(actions);

    auto* output = new QTextEdit();
    output->setReadOnly(true);
    output->setMinimumHeight(180);
    contentLayout->addWidget(makeCard("运行输出", "stdout / stderr", output));
    contentLayout->addStretch();

    QObject::connect(runBtn, &QPushButton::clicked, [&]() {
        status->setText("正在运行...");
        transLabel->setText("计算中...");
        output->clear();
        QString exe = defaultExePath;
        if (advancedBox->isChecked()) {
            QString customExe = exePath->text().trimmed();
            if (!customExe.isEmpty()) {
                exe = customExe;
            }
        }
        if (exe.isEmpty()) {
            QMessageBox::warning(&window, "输入错误", "请填写 atm.exe 路径");
            return;
        }

        QStringList lines;
        lines << (rh->text().trimmed() + " " + tempC->text().trimmed());

        bool isHorizontal = (h1->text().trimmed() == h2->text().trimmed());
        lines << (isHorizontal ? "0" : "1");
        if (isHorizontal) {
            lines << (h1->text().trimmed() + " " + ground->text().trimmed());
        } else {
            lines << (h1->text().trimmed() + " " + h2->text().trimmed() + " " + ground->text().trimmed());
        }

        if (advancedBox->isChecked()) {
            lines << f0->text().trimmed();
            lines << co2ppm->text().trimmed();
            lines << scaleByMix->text().trimmed();
            lines << (startUm->text().trimmed() + " " + endUm->text().trimmed() + " " + stepUm->text().trimmed());
            lines << h2oScale->text().trimmed();
            lines << co2Scale->text().trimmed();
            lines << h2oAltScale->text().trimmed();
            lines << h2oCsv->text().trimmed();
            lines << co2Csv->text().trimmed();
        } else {
            lines << "6.8";
            lines << "400";
            lines << "1";
            lines << "3 5 0.05";
            lines << "-1";
            lines << "-1";
            lines << "-1";
            lines << "-";
            lines << "-";
        }
        QString input = lines.join("\n") + "\n";

        QProcess process;
        process.setProgram(exe);
        process.start();
        if (!process.waitForStarted()) {
            status->setText("启动失败");
            QMessageBox::critical(&window, "启动失败", "无法启动 atm.exe");
            return;
        }
        process.write(input.toUtf8());
        process.closeWriteChannel();
        process.waitForFinished();

        QString out = process.readAllStandardOutput();
        QString err = process.readAllStandardError();
        output->append("[stdout]\n" + out.trimmed());
        if (!err.isEmpty()) {
            output->append("\n[stderr]\n" + err.trimmed());
        }

        QString value;
        const QString key = "Band-average transmittance:";
        
        QList<QPointF> curveTrans;
        QList<QPointF> curveH2O;
        QList<QPointF> curveCO2;
        bool inCurveData = false;

        for (const auto& line : out.split('\n')) {
            if (line.contains(key)) {
                value = line.section(':', 1).trimmed();
            } else if (line.contains("===== CURVE DATA =====")) {
                inCurveData = true;
            } else if (line.contains("===== END CURVE DATA =====")) {
                inCurveData = false;
            } else if (inCurveData) {
                QString trimmed = line.trimmed();
                if (!trimmed.isEmpty() && !trimmed.startsWith("Wavelength")) {
                    auto parts = trimmed.split(',');
                    if (parts.size() >= 2) {
                        bool ok1, ok2;
                        double x = parts[0].toDouble(&ok1);
                        double y = parts[1].toDouble(&ok2);
                        if (ok1 && ok2) {
                            curveTrans.append(QPointF(x, y));
                        }
                    }
                    if (parts.size() >= 4) {
                        bool okh, okc;
                        double h2o = parts[2].toDouble(&okh);
                        double co2 = parts[3].toDouble(&okc);
                        if (okh && okc) {
                            curveH2O.append(QPointF(parts[0].toDouble(), h2o));
                            curveCO2.append(QPointF(parts[0].toDouble(), co2));
                        }
                    }
                }
            }
        }

        auto applyCurve = [](QChart* c, const QList<QPointF>& data, const QString& yLabel) {
            c->removeAllSeries();
            auto axes = c->axes();
            for (auto* ax : axes) {
                c->removeAxis(ax);
            }

            if (data.isEmpty()) {
                c->setTitle("未生成有效数据");
                return;
            }

            double minX = data.first().x();
            double maxX = data.first().x();
            double minY = data.first().y();
            double maxY = data.first().y();
            for (const auto& pt : data) {
                if (pt.x() < minX) minX = pt.x();
                if (pt.x() > maxX) maxX = pt.x();
                if (pt.y() < minY) minY = pt.y();
                if (pt.y() > maxY) maxY = pt.y();
            }

            auto* series = new QLineSeries();
            QPen pen(QColor("#49c1ff"));
            pen.setWidth(2);
            series->setPen(pen);
            series->append(data);
            c->addSeries(series);

            auto* axisX = new QValueAxis();
            axisX->setTitleText("Wavelength (μm)");
            axisX->setLabelFormat("%.2f");
            axisX->setRange(minX, maxX);
            axisX->setLabelsColor(QColor("#a9bdc9"));
            axisX->setLinePenColor(QColor("#2d4651"));
            axisX->setGridLineColor(QColor("#1f313b"));

            auto* axisY = new QValueAxis();
            axisY->setTitleText(yLabel);
            if (yLabel == "Transmittance") {
                axisY->setRange(0.0, 1.0);
            } else {
                axisY->setRange(minY, maxY);
            }
            axisY->setLabelsColor(QColor("#a9bdc9"));
            axisY->setLinePenColor(QColor("#2d4651"));
            axisY->setGridLineColor(QColor("#1f313b"));

            c->addAxis(axisX, Qt::AlignBottom);
            c->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisX);
            series->attachAxis(axisY);

            c->setTitle("");
        };

        applyCurve(chartTrans, curveTrans, "Transmittance");
        applyCurve(chartH2O, curveH2O, "Alpha (cm^-1)");
        applyCurve(chartCO2, curveCO2, "Alpha (cm^-1)");

        if (value.isEmpty()) {
            status->setText("未找到结果");
            transLabel->setText("平均透过率: --");
            QString detail = out;
            if (!err.isEmpty()) {
                detail += "\n[stderr]\n" + err;
            }
            QMessageBox::warning(&window, "未找到结果", detail);
            return;
        }
        status->setText("完成");
        transLabel->setText("平均透过率: " + value);
    });

    window.resize(1100, 860);
    window.show();

    return app.exec();
}
