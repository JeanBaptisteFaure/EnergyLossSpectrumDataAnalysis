#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <qcustomplot.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
//    void makePlot();

    void on_PlotButton_clicked();

    void on_DirButton_clicked();

    void on_listWidget_itemClicked();

    void Plot(QMap<QString, QString> header, QList<QVector<double>> counts, QStringList plotnames, QString plotstyle = "scatter");

    void EnergyPlot(QMap<QString, QString> header, QList<QVector<double>> counts, QStringList plotnames, QString plotstyle = "scatter");

//    void on_LineCursorButton_clicked();

    void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);
//    void showCursor(QMouseEvent *event);

    void on_CalibrateButton_clicked();

    void on_BinsButton_clicked();

    void on_InstructionsButton_clicked();

    void on_EnergyWidget_itemClicked();

    void on_LSFit_clicked();

    void on_BackgroundSelectButton_clicked();

    void on_BaselineButton_clicked();

    void on_SinglePeakCalibrateButton_clicked();

    void on_FitRegion_clicked();

    void on_GaussianButton_clicked();

    void on_FitGaussianButton_clicked();

    void on_TwoPeakButton_clicked();

    void on_SinglePeakButton_clicked();

    void on_ReduceYButton_clicked();

    void on_IncreaseYButton_clicked();

    void on_ReduceXButton_clicked();

    void on_IncreaseXButton_clicked();

    void on_SaveButton_clicked();


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
