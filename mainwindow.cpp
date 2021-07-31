#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "math.h"
#include<cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: #E0E0E0;");
    //hide scale buttons group box
//    ui->ScaleBox->hide();
    //set default checks for plotbox:
    ui->RawCheck->setChecked(true);
    ui->BackgroundCheck->setChecked(true);
    ui->TrueCheck->setChecked(true);
    ui->ScatterCheck->setChecked(true);
    //set colors for the plot
    ui->customPlot->setBackground(QBrush(Qt::black));
    ui->customPlot->xAxis->setTickLabelColor(Qt::white);
    ui->customPlot->xAxis->setBasePen(QPen(Qt::white));
    ui->customPlot->xAxis->setLabelColor(Qt::white);
    ui->customPlot->xAxis->setTickPen(QPen(Qt::white));
    ui->customPlot->xAxis->setSubTickPen(QPen(Qt::white));
    ui->customPlot->yAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis->setBasePen(QPen(Qt::white));
    ui->customPlot->yAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis->setTickPen(QPen(Qt::white));
    ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white));


    // double click event to find data point
    connect(ui->customPlot,SIGNAL(plottableDoubleClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}


// variables:
QString gdirString;
QMap<QString, QString> gheader;
QVector<double> grawcounts;
QVector<double> gbackgroundcounts;
QVector<double> gtruecounts;
int gplotsize;
QStringList gplotnames;
QVector<double> gbinTracker;

// Scale Factors:
double gyScaleFactorUp = 1;
double gyScaleFactorDown = 1;
double gxScaleFactorUp = 1;
double gxScaleFactorDown = 1;

//for my final fit:
QStringList gEnergiesList;
double gsigma2 = 0;
//track if I'm fitting peaks
int gsinglepeak = 0;
int gtwopeak = 0;

//track if I'm fitting or making a baseline
int gclickCounter = 0;
int gbaseline = 0;
QVector<int> gbaselineTracker;

//gaussian tracker
int ggaussian;
QVector<int> ggaussianTracker;
QVector<double> ggaussianfit;

//fitting region tracker
int gfit = 0;
QVector<int> gfitTracker;

double gEnergyPerBin = 0;
QVector<double> gEnergyVector;
//to keep track of our plot as header or energy, we will set this global to be 0 (Energy) and 1 (Bins)
int gBinsPlot = 1;
//for the theoretical energies
QVector<double> gTheoEnergy;
int gtheoIgnore = 0; //ignore theoretical values when plotting on 1;

// globals for csv
QVector<double> gpeakposition;
QVector<double> gmaxheight;
QVector<double> garea;
QVector<double> gchi2;
double gchi2Final = 0;


//Bins Plotting Function
void MainWindow::Plot(QMap<QString, QString> header, QList<QVector<double>> counts, QStringList plotnames, QString plotstyle)
{
    ui->customPlot->legend->clearItems();
//    ui->customPlot->clearGraphs();
    gplotnames = plotnames;
    gplotsize = counts.size();
    //Find x-axis in terms of Bins
    QVector<double> binvalues;
    for(int i=0;i<header["Bins"].toInt();i++)
    {
        binvalues.append(i);
    }

    // find maximum y-axis value of counts
    double maxcounts = counts[0][0];
    for(int j=0;j<counts.size();j++)
    {
        for(int i=1;i<counts[j].size();i++)
        {
            if(maxcounts<counts[j][i])
            {
                maxcounts = counts[j][i];
            }
        }
    }


    // create graph and assign data to it:
    ui->customPlot->setInteraction(QCP::iRangeDrag, true);
    ui->customPlot->setInteraction(QCP::iRangeZoom, true);

    for(int i=0;i<counts.size();i++)
    {
        ui->customPlot->addGraph();
        if(plotstyle=="scatter")
        {
            ui->customPlot->graph(i)->setLineStyle(QCPGraph::lsNone);
        }
        else
        {
            ui->customPlot->graph(i)->setLineStyle(QCPGraph::lsLine);
        }
    }

    for(int i=0;i<counts.size();i++)
    {
        QCPScatterStyle scatter;
        scatter.setShape(QCPScatterStyle::ssDisc);
        scatter.setSize(4);
        ui->customPlot->graph(i)->setScatterStyle(scatter);
        ui->customPlot->graph(i)->setData(binvalues, counts[i]);
        if(plotnames[i]=="raw")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::red));
            ui->customPlot->graph(i)->setName("Raw");
        }
        else if(plotnames[i]=="background")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::blue));
            ui->customPlot->graph(i)->setName("Background");
        }
        else if(plotnames[i]=="true")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::green));
            ui->customPlot->graph(i)->setName("True");
        }

    }


    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("Bins");
    ui->customPlot->yAxis->setLabel("Counts");
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0, binvalues.size());
    ui->customPlot->yAxis->setRange(0, maxcounts);
    ui->customPlot->legend->setVisible(true);
    ui ->customPlot->replot();

}
// Energy Plotting Function
void MainWindow::EnergyPlot(QMap<QString, QString> header, QList<QVector<double>> counts, QStringList plotnames, QString plotstyle)
{

    ui->customPlot->legend->clearItems();
//    ui->customPlot->clearGraphs();
    gplotnames = plotnames;
    gplotsize = counts.size();
    //Find x-axis in terms of Energy
    QVector<double> EnergyVector;
    //set the proper increments and shift the plot
    double smallEnergy;
    double smallBin;
    double currentEnergy = 0;
    if (gtheoIgnore == 0)
    {
        if(gTheoEnergy[0]<gTheoEnergy[1])
        {
            smallEnergy = gTheoEnergy[0];
        }
        else
        {
            smallEnergy = gTheoEnergy[1];
        }
        if(gbinTracker[0]<gbinTracker[1])
        {
            smallBin = gbinTracker[0];
        }
        else
        {
            smallBin = gbinTracker[1];
        }
        currentEnergy = smallEnergy - smallBin*gEnergyPerBin;
    }

    if(gtheoIgnore == 1)
    {
        smallEnergy = gTheoEnergy[0];
        smallBin = gbinTracker[0];
        currentEnergy = smallEnergy - smallBin*gEnergyPerBin;
    }
    for(int i=0;i<header["Bins"].toInt();i++)
    {
        EnergyVector.append(currentEnergy);
        currentEnergy +=gEnergyPerBin;
    }
    gEnergyVector = EnergyVector;


    // find maximum y-axis value of counts
    double maxcounts = counts[0][0];
    for(int j=0;j<counts.size();j++)
    {
        for(int i=1;i<counts[j].size();i++)
        {
            if(maxcounts<counts[j][i])
            {
                maxcounts = counts[j][i];
            }
        }
    }


    // create graph and assign data to it:
    ui->customPlot->setInteraction(QCP::iRangeDrag, true);
    ui->customPlot->setInteraction(QCP::iRangeZoom, true);

    for(int i=0;i<counts.size();i++)
    {
        ui->customPlot->addGraph();
        if(plotstyle=="scatter")
        {
            ui->customPlot->graph(i)->setLineStyle(QCPGraph::lsNone);
        }
        else
        {
            ui->customPlot->graph(i)->setLineStyle(QCPGraph::lsLine);
        }
    }

    for(int i=0;i<counts.size();i++)
    {
        QCPScatterStyle scatter;
        scatter.setShape(QCPScatterStyle::ssDisc);
        scatter.setSize(4);
        ui->customPlot->graph(i)->setScatterStyle(scatter);
        ui->customPlot->graph(i)->setData(EnergyVector, counts[i]);
        if(plotnames[i]=="raw")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::red));
            ui->customPlot->graph(i)->setName("Raw");
        }
        else if(plotnames[i]=="background")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::blue));
            ui->customPlot->graph(i)->setName("Background");
        }
        else if(plotnames[i]=="true")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::green));
            ui->customPlot->graph(i)->setName("True");
        }
        else if(plotnames[i] == "gauss")
        {
            ui->customPlot->graph(i)->setPen(QPen(Qt::white));
            ui->customPlot->graph(i)->setName("Gaussian");
        }

    }


    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("Energy (eV)");
    ui->customPlot->yAxis->setLabel("Counts");
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(EnergyVector[0], EnergyVector[EnergyVector.size()-1]);
    ui->customPlot->yAxis->setRange(0, maxcounts);
    ui->customPlot->legend->setVisible(true);
    ui->customPlot->yAxis->scaleRange(pow(.9,gyScaleFactorDown));
    ui->customPlot->yAxis->scaleRange(pow(1.1,gyScaleFactorUp));
    ui->customPlot->replot();
    ui ->customPlot->replot();
}

//Vertical Line on doubleclick function
//In order to find point. I used this excellent example https://www.qcustomplot.com/index.php/demos/interactionexample
void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
    if(gtwopeak == 1)
    {
        if(gclickCounter == 0)
        {
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("yellow"));
            line1->point1->setCoords(dataIndex,0);
            line1->point2->setCoords(dataIndex,dataValue);
            gbinTracker.append(dataIndex);
            gclickCounter ++;
            ui->customPlot->replot();
        }
        else if(gclickCounter ==1 )
        {
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line2 = new QCPItemStraightLine(ui->customPlot);
            line2->setPen(QPen("yellow"));
            line2->point1->setCoords(dataIndex,0);
            line2->point2->setCoords(dataIndex,dataValue);
            gbinTracker.append(dataIndex);
            gclickCounter ++;
            ui->customPlot->replot();

        }
        else if(gclickCounter==2)
        {
            ui->customPlot->clearItems();
            ui->customPlot->replot();
            gclickCounter = 0;
            gbinTracker.clear();
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("yellow"));
            line1->point1->setCoords(dataIndex,0);
            line1->point2->setCoords(dataIndex,dataValue);
            gbinTracker.append(dataIndex);
            gclickCounter ++;
            ui->customPlot->replot();

        }
    }
    else if(gsinglepeak == 1)
    {
        if(gclickCounter == 0)
        {
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("yellow"));
            line1->point1->setCoords(dataIndex,0);
            line1->point2->setCoords(dataIndex,dataValue);
            gbinTracker.append(dataIndex);
            gclickCounter ++;
            ui->customPlot->replot();
        }
        else if(gclickCounter ==1 )
        {
            ui->customPlot->clearItems();
            ui->customPlot->replot();
            gclickCounter = 0;
            gbinTracker.clear();
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line2 = new QCPItemStraightLine(ui->customPlot);
            line2->setPen(QPen("yellow"));
            line2->point1->setCoords(dataIndex,0);
            line2->point2->setCoords(dataIndex,dataValue);
            gbinTracker.append(dataIndex);
            gclickCounter ++;
            ui->customPlot->replot();
        }
    }
    else if(gbaseline == 1)
    {
        gclickCounter = 2;
        double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
        QString message;
        if(gBinsPlot == 0)
        {
            message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
        }
        else if(gBinsPlot == 1)
        {
            message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
        }
        ui->statusbar->showMessage(message, 10000);
        QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
        line1->setPen(QPen("purple"));
        line1->point1->setCoords(dataIndex,0);
        line1->point2->setCoords(dataIndex,dataValue);
        gbaselineTracker.append(dataIndex);
        ui->customPlot->replot();
    }
    else if(gfit == 1)
    {
        gclickCounter = 2;
        if(gfitTracker.size()<2)
        {            
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("red"));
            line1->point1->setCoords(gEnergyVector[dataIndex],0);
            line1->point2->setCoords(gEnergyVector[dataIndex],dataValue);
            gfitTracker.append(dataIndex);
            ui->customPlot->replot();
        }
        else if(gfitTracker.size()>=2)
        {
            ui->customPlot->clearItems();
            ui->customPlot->replot();
            gfitTracker.clear();
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("red"));
            line1->point1->setCoords(gEnergyVector[dataIndex],0);
            line1->point2->setCoords(gEnergyVector[dataIndex],dataValue);
            gfitTracker.append(dataIndex);
            ui->customPlot->replot();
        }
    }
    else if(ggaussian == 1)
    {
        gclickCounter = 2;
        if(ggaussianTracker.size()<2)
        {
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("blue"));
            line1->point1->setCoords(gEnergyVector[dataIndex],0);
            line1->point2->setCoords(gEnergyVector[dataIndex],dataValue);
            ggaussianTracker.append(dataIndex);
//            ggaussianTracker.append(gEnergyVector[dataIndex]);
            ui->customPlot->replot();
        }
        else if(ggaussianTracker.size()>=2)
        {
            ui->customPlot->clearItems();
            ui->customPlot->replot();
            ggaussianTracker.clear();
            double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
            QString message;
            if(gBinsPlot == 0)
            {
                message = QString("Clicked on graph '%1' at energy value %2 eV with counts %3.").arg(plottable->name()).arg(gEnergyVector[dataIndex]).arg(dataValue);
            }
            else if(gBinsPlot == 1)
            {
                message = QString("Clicked on graph '%1' at bin value #%2 with counts %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
            }
            ui->statusbar->showMessage(message, 10000);
            QCPItemStraightLine *line1 = new QCPItemStraightLine(ui->customPlot);
            line1->setPen(QPen("blue"));
            line1->point1->setCoords(gEnergyVector[dataIndex],0);
            line1->point2->setCoords(gEnergyVector[dataIndex],dataValue);
            ggaussianTracker.append(dataIndex);
            ui->customPlot->replot();
        }
    }
    else
    {
        ;
    }
}

//instructions on plotting based on plot type selections
void MainWindow::on_PlotButton_clicked()
{
    ui->customPlot->clearGraphs();
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No Directory Selected. Please Select a Directory.");
        error_nodir.exec();
    }
    else if(gBinsPlot == 1)
    {
        if(ui->ScatterCheck->isChecked())
        {
            if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gbackgroundcounts, gtruecounts}, {"raw", "background", "true"});
            }

            else if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gbackgroundcounts}, {"raw", "background"});
            }

            else if(ui->RawCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gtruecounts}, {"raw", "true"});
            }

            else if(ui->TrueCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gtruecounts, gbackgroundcounts}, {"true", "background"});
            }

            else if(ui->RawCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts}, {"raw"});
            }

            else if(ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gbackgroundcounts}, {"background"});
            }

            else if(ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gtruecounts}, {"true"});
            }
        }

        else if(ui->LineCheck->isChecked())
        {
            if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gbackgroundcounts, gtruecounts}, {"raw", "background", "true"}, "line");
            }

            else if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gbackgroundcounts}, {"raw", "background"}, "line");
            }

            else if(ui->RawCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts, gtruecounts}, {"raw", "true"}, "line");
            }

            else if(ui->TrueCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gtruecounts, gbackgroundcounts}, {"true", "background"}, "line");
            }

            else if(ui->RawCheck->isChecked())
            {
                MainWindow::Plot(gheader, {grawcounts}, {"raw"}, "line");
            }

            else if(ui->BackgroundCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gbackgroundcounts}, {"background"}, "line");
            }

            else if(ui->TrueCheck->isChecked())
            {
                MainWindow::Plot(gheader, {gtruecounts}, {"true"}, "line");
            }
        }
    }

    else if(gBinsPlot == 0)
    {
        if(ui->ScatterCheck->isChecked())
        {
            if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gbackgroundcounts, gtruecounts}, {"raw", "background", "true"});
            }

            else if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gbackgroundcounts}, {"raw", "background"});
            }

            else if(ui->RawCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gtruecounts}, {"raw", "true"});
            }

            else if(ui->TrueCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gtruecounts, gbackgroundcounts}, {"true", "background"});
            }

            else if(ui->RawCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts}, {"raw"});
            }

            else if(ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gbackgroundcounts}, {"background"});
            }

            else if(ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gtruecounts}, {"true"});
            }
        }

        else if(ui->LineCheck->isChecked())
        {
            if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gbackgroundcounts, gtruecounts}, {"raw", "background", "true"}, "line");
            }

            else if(ui->RawCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gbackgroundcounts}, {"raw", "background"}, "line");
            }

            else if(ui->RawCheck->isChecked() && ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts, gtruecounts}, {"raw", "true"}, "line");
            }

            else if(ui->TrueCheck->isChecked() && ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gtruecounts, gbackgroundcounts}, {"true", "background"}, "line");
            }

            else if(ui->RawCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {grawcounts}, {"raw"}, "line");
            }

            else if(ui->BackgroundCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gbackgroundcounts}, {"background"}, "line");
            }

            else if(ui->TrueCheck->isChecked())
            {
                MainWindow::EnergyPlot(gheader, {gtruecounts}, {"true"}, "line");
            }
        }
    }

    else
    {
        QMessageBox error_noCheck;
        error_noCheck.setText("Invalid selection. Please select at least one plot and one plot type.");
        error_noCheck.exec();
    }



}

//Select Directory
void MainWindow::on_DirButton_clicked()
{
    //if opening new directory, clear previous widgets
    ui->listWidget->clear();
    ui->EnergyWidget->clear();
    ui->customPlot->clearGraphs();
    ui->customPlot->clearItems();
    ui->customPlot->replot();

    //Get the directory
    QDir dirName = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        "",
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    //get a clean list of files from the directory
    QStringList filelist = dirName.entryList(QStringList() << "*.ON1", QDir::Files);
    for(int i=0;i<filelist.size();i++)
    {
        filelist[i].remove(filelist[i].size()-4, 4);
    }
    //put list in widget
    ui -> listWidget ->addItems(filelist);
    ui->listWidget->setAlternatingRowColors(true);
    QString dirString;
    dirString = dirName.path();
    gdirString = dirString;

    //extract the energies file:
    QStringList EnergyFileList = dirName.entryList(QStringList() << "*.ERG", QDir::Files);
    if(EnergyFileList.isEmpty())
    {
        ;
    }
    else
    {
        QString EnergyFilePath = dirString.append("\\") + EnergyFileList[0];
        QFile EnergyFile(EnergyFilePath);
        EnergyFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream EnergyStream(&EnergyFile);
        QString EnergyString = EnergyStream.readAll();
        QStringList EnergyStringList = EnergyString.split("\n", QString::SkipEmptyParts);
        ui->EnergyWidget->addItems(EnergyStringList);
        ui->EnergyWidget->setAlternatingRowColors(true);
        gEnergiesList = EnergyStringList;
    }
}

//Selecting the proper .ON0 file
void MainWindow::on_listWidget_itemClicked()
{
    grawcounts.clear();
    gbackgroundcounts.clear();
    gtruecounts.clear();
    ui->customPlot->clearItems();
    ui->customPlot->replot();
    gBinsPlot = 1;
    gsinglepeak = 0;
    gtwopeak = 0;
    gbaseline = 0;
    gfit = 0;
    ggaussian = 0;
    gclickCounter = 0;
    // convert selected file to pathnames
    QString file = ui->listWidget->currentItem()->text();
    QString fullfile = gdirString.append("\\") + file;
    QString backgroundpath = fullfile;
    backgroundpath.append(".ON0");
    QString rawpath = fullfile;
    rawpath.append(".ON1");

    //convert raw file to a list of strings
    QFile rawfile(rawpath);
    rawfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream rawstream(&rawfile);
    QString rawstring = rawstream.readAll();
    QStringList rawstringList = rawstring.split("\n",QString::SkipEmptyParts);

    // split raw into header and counts
    QVector<double> rawcounts;
    QMap<QString, QString> header;

    for(int i=0;i<rawstringList.size();i++)
    {
        if(rawstringList[i][0].isDigit())
        {
            rawcounts.append(rawstringList[i].toDouble());
        }
        else if(rawstringList[i][0].isLetter())
        {
            QStringList mapping = rawstringList[i].split('=');
            mapping[0] = mapping[0].trimmed();
            mapping[1] = mapping[1].trimmed();
            header[mapping[0]] = mapping[1];
        }
    }
    grawcounts = rawcounts;
    gheader = header;

    // Make Properties tab
    ui->headerWidget->setRowCount(header.size());
    ui->headerWidget->setColumnCount(2);
    ui->headerWidget->horizontalHeader()->setStretchLastSection(true);
    ui->headerWidget->setAlternatingRowColors(true);
    ui->headerWidget->horizontalHeader()->hide();
    ui->headerWidget->verticalHeader()->hide();
    QStringList headerkeys = header.keys();
    for(int i=0;i<header.size();i++)
    {
        ui->headerWidget->setItem(i,0,new QTableWidgetItem(headerkeys[i]));
        ui->headerWidget->setItem(i,1,new QTableWidgetItem(header[headerkeys[i]]));
    }

    //Get energies per bin
    double totalEnergy = header["End_V"].toDouble();
    double totalBins = header["Bins"].toDouble();
    double EnergyPerBin = totalEnergy/totalBins;
    gEnergyPerBin = EnergyPerBin;

    // get background counts if .ON0 available
    if(QFile::exists(backgroundpath))
    {
        //make a background list of strings
        QFile backgroundfile(backgroundpath);
        backgroundfile.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream backgroundstream(&backgroundfile);
        QString backgroundstring = backgroundstream.readAll();
        QStringList backgroundstringList = backgroundstring.split("\n",QString::SkipEmptyParts);

        //get backgound counts
        QVector<double> backgroundcounts;
        for(int i=0;i<backgroundstringList.size();i++)
        {
            if(backgroundstringList[i][0].isDigit())
            {
                backgroundcounts.append(backgroundstringList[i].toDouble());
            }
        }
        gbackgroundcounts = backgroundcounts;
        QVector<double> truecounts;
        for(int i=0;i<backgroundcounts.size();i++)
        {
            truecounts.append(rawcounts[i]-backgroundcounts[i]);
        }
        gtruecounts = truecounts;
    }

    MainWindow::on_PlotButton_clicked();
}

//Calibrating bins to energy
void MainWindow::on_CalibrateButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gclickCounter != 2)
    {
        QMessageBox error_noclick;
        error_noclick.setText("Please ensure you have selected two calibration lines.");
        error_noclick.exec();
    }
    else if(gTheoEnergy.size() != 2)
    {
        QMessageBox error_noEnergy;
        error_noEnergy.setText("Please ensure you have two selected items in the 'Energies' tab.");
        error_noEnergy.exec();
    }
    else
    {
        double newEnergy = fabs(gTheoEnergy[1]-gTheoEnergy[0]);
        double newBins = fabs(gbinTracker[1]-gbinTracker[0]);
        gEnergyPerBin = newEnergy/newBins;
        gBinsPlot = 0;
        ui->customPlot->clearItems();
        gtheoIgnore = 0;
        on_PlotButton_clicked();
        QString message;
        message = QString("Energy Start = '%1' eV with counts '%'").arg(gEnergyVector[0]);
        ui->statusbar->showMessage(message, 10000);
    }
}
//reset to bins values
void MainWindow::on_BinsButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gBinsPlot = 1;
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gclickCounter = 0;
        on_PlotButton_clicked();
        gEnergyVector.clear();
        gEnergyPerBin = gheader["Energy"].toDouble()/gheader["Bins"].toDouble();
    }

}
// Display instructions
void MainWindow::on_InstructionsButton_clicked()
{
    QMessageBox instructions;
    instructions.setText("Starting up:"
"\n Begin by clicking 'Select Directory' and select the directory where your .ON0, ON1, and Energy files are."
"\n Next select a file under the 'Files' tab in order to load the data. If you wish to switch the file you are looking at, simply click on the desired file."
"\n\n Selecting Plot type:"
"\n In the 'Select Plot Type' box, you can choose the kind of file you wish to see, as well as decide between scatter or line styles. At any time you can change these settings."
"\n\n Plot Navigation: "
"\n Click, hold, and drag in order to move the plot."
"\n Scroll to zoom."
"\n Use arrow keys to adjust axes: Left/Right = contraxt/expand x-axis; Down/Up = contract/expand y-axis. You can also press and hold."
"\n Select the 'Properties' tab in order to view the properties of the selected file."
"\n Select 'Reset' in order to reset the plot."
"\n When selecting points on the plot, information about the point will appear at the bottom left of the window."
"\n If a double click is not registering, you are not clicking close enough to a point. Zoom in and click more closely to the point you wish to select."
"\n\n Generating a Background: "
"\n If you wish to generate a background for your raw file, click 'Select Background'. You may now double click points on the plot to mark regions as background. Once all regions haev been selected, click 'Create Baseline'."
"\n\n Single Peak Calibration:"
"\n If you wish to calibrate with a single peak, click 'Select Peak'. You may now double click on a point on the plot in order to select a peak. Then go to the 'Energies' tab and select the energy of that peak. Lastly, input an energy per bin and click 'calibrate'."
"\n\n Two Peak Calibration:"
"\n If you wish to calibrate with two peaks, click 'Select Peak'. You may now double click points on the plot in order to select your peaks. Then go to the 'Energies' tab and select the energies of those peaks (order does not matter). Lastly, click 'calibrate'."
"\n\n Fitting the Spectrum:"
"\n Once calibrated, you will mark the best inelastic gaussian. First, click 'Mark Gaussian Region' Then double click points on the plot on both sides of the gaussian to select it. Lastly, click 'Fit Gaussian'."
"\n Once the gassian is fitted, you can fit the entire plot. Click 'Select Fitting Region'. Then double click points on the plot to select the region you wish to fit. Lastly, click 'Fit Region'."
"NOTE: When selecting the fitting region, click ont TRUE points (green), not the gaussian points (white) if you want to see the red lines denoting the selected region. Program will work regardless, this is just a display bug."
);
    instructions.exec();
}

//Select the energy values for calibration
void MainWindow::on_EnergyWidget_itemClicked()
{
    QString energy = ui->EnergyWidget->currentItem()->text();
    double EnergyValue = energy.toDouble();
    if(gTheoEnergy.isEmpty())
    {
        gTheoEnergy.append(EnergyValue);
//        ui->EnergyWidget->currentItem()->setBackgroundColor(QColor("Blue"));
        ui->EnergyWidget->currentItem()->setTextColor(QColor("blue"));
    }
    else if(gTheoEnergy.size() == 1)
    {
        gTheoEnergy.append(EnergyValue);
        ui->EnergyWidget->currentItem()->setTextColor("blue");
    }
    else if(gTheoEnergy.size() == 2)
    {
        for(int i=0;i<ui->EnergyWidget->count();i++)
        {
            ui->EnergyWidget->item(i)->setTextColor("black");
        }
        gTheoEnergy.clear();
        gTheoEnergy.append(EnergyValue);
        ui->EnergyWidget->currentItem()->setTextColor("blue");
    }
    else
    {
        QMessageBox EnergySelect;
        EnergySelect.setText("Something not good");
        EnergySelect.exec();
    }

}

//Linear Fit WIP
void MainWindow::on_LSFit_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gEnergyVector.isEmpty())
    {
        QMessageBox error_nocal;
        error_nocal.setText("Not yet calibrated. Please calibrate before fitting.");
        error_nocal.exec();
    }
    else if(gsigma2 == 0)
    {
        QMessageBox error_nosig;
        error_nosig.setText("No gaussian attempted. Please first fit a gaussian.");
        error_nosig.exec();
    }
    else
    {
        QVector<double> peakposition, gaussfit, gaussfitTemp, maxheight,area, heightpositionVector;
        double difference, heightpositionTracker, areanum, truemain, truetemp;
        int firstPosition, secondPosition;
        if(gfitTracker[0]<gfitTracker[1])
        {
            firstPosition = gfitTracker[0];
            secondPosition = gfitTracker[1];
        }
        if(gfitTracker[0]>gfitTracker[1])
        {
            firstPosition = gfitTracker[1];
            secondPosition = gfitTracker[0];
        }

        for(int i=0;i<gEnergiesList.size();i++)
        {
            if(gEnergiesList[i].toDouble()>=gEnergyVector[firstPosition] && gEnergiesList[i].toDouble()<=gEnergyVector[secondPosition])
            {
                peakposition.append(gEnergiesList[i].toDouble());
            }
        }

        gpeakposition = peakposition;
        for(int j=0;j<peakposition.size();j++)
        {
            heightpositionTracker = j;
            difference = 100000000;
            for(int i=0;i<gEnergyVector.size();i++)
            {
                if(fabs(peakposition[j]-gEnergyVector[i])<difference)
                {
                    difference = fabs(peakposition[j]-gEnergyVector[i]);
                    heightpositionTracker = i;
                }
                if(fabs(peakposition[j]-gEnergyVector[i])>=difference)
                {
                    ;
                }
            }
            heightpositionVector.append(heightpositionTracker);
            if(gtruecounts[heightpositionTracker]<0)
            {
                maxheight.append(0);
            }
            if(gtruecounts[heightpositionTracker]>=0)
            {
                maxheight.append(gtruecounts[heightpositionTracker]);
            }

        }

        gmaxheight = maxheight;
        for(int j=0;j<peakposition.size();j++)
        {
            //gaussian of position j
            if(gaussfit.isEmpty())
            {
                for(int i=0;i<gEnergyVector.size();i++)
                {
                    gaussfit.append(maxheight[j]*exp(-pow(gEnergyVector[i]-peakposition[j],2)/gsigma2));
                }
                // finding chi squared

                //find right limit of gaussian
                int rightpositionTracker = gfitTracker[1];
                for(int i=heightpositionVector[j];i<gfitTracker[1];i++)
                {
                    if(gaussfit[i]<=1)
                    {
                        rightpositionTracker = i;
                        break;
                    }
                }
                //find left limit of gaussian
                int leftpositionTracker = gfitTracker[0];
                for(int i=heightpositionVector[j];i>gfitTracker[0];i--)
                {
                    if(gaussfit[i]<=1)
                    {
                        leftpositionTracker = i;
                        break;
                    }
                }

                double chi2top = 0, chi2bot = 0, chi2 = 0;
                for(int i=leftpositionTracker;i<rightpositionTracker;i++)
                {
                    chi2top += pow(fabs(gaussfit[i]-gtruecounts[i]),2);
                    chi2bot += gaussfit[i];
                }
                chi2 = chi2top/(4*chi2bot);
                gchi2.append(chi2);

                //find area under curve by summing counts
                areanum = 0;
                for(int k=0;k<gaussfit.size();k++)
                {
                    areanum += gaussfit[k];
                }
                area.append(areanum);
            }
            else
            {
                gaussfitTemp.clear();
                for(int i=0;i<gEnergyVector.size();i++)
                {
                    gaussfitTemp.append(maxheight[j]*exp(-pow(gEnergyVector[i]-peakposition[j],2)/gsigma2));
                }

                // finding chi squared

                //find right limit of gaussian
                int rightpositionTracker = gfitTracker[1];
                for(int i=heightpositionVector[j];i<gfitTracker[1];i++)
                {
                    if(gaussfitTemp[i]<=1)
                    {
                        rightpositionTracker = i;
                        break;
                    }
                }
                //find left limit of gaussian
                int leftpositionTracker = gfitTracker[0];
                for(int i=heightpositionVector[j];i>gfitTracker[0];i--)
                {
//                    if(gaussfitTemp[i]<=maxheight[j]/10000)
                    if(gaussfitTemp[i]<=1)
                    {
                        leftpositionTracker = i;
                        break;
                    }
                }
                double chi2top = 0, chi2bot = 0, chi2 = 0;
                for(int i=leftpositionTracker;i<rightpositionTracker;i++)
                {
                    chi2top += pow(fabs(gaussfitTemp[i]-gtruecounts[i]),2);
                    chi2bot += gaussfitTemp[i];
                }
                chi2 = chi2top/(4*chi2bot);
                gchi2.append(chi2);

                //area under the curve
                areanum = 0;
                for(int k=0;k<gaussfitTemp.size();k++)
                {
                    areanum += gaussfitTemp[k];
                }
                area.append(areanum);


                // now we combine all of the separate gaussians into one for the visual of the fit.
                for(int n=0;n<gaussfitTemp.size();n++)
                {
                    truemain = fabs(gaussfit[n]-gtruecounts[n]);
                    truetemp = fabs(gaussfitTemp[n]-gtruecounts[n]);
                    if(truemain<truetemp)
                    {
                        ;
                    }
                    if(truemain>=truetemp)
                    {
                        gaussfit[n]=gaussfitTemp[n];
                    }
                }
            }
        }
        ggaussianfit = gaussfit;
        garea = area;

        // Now we do a Chi squared of the entire fit:
        double chi2top = 0, chi2bot = 0, chi2Final = 0;
        for(int i=gfitTracker[0];i<gfitTracker[1];i++)
        {
            chi2top += pow(fabs(gaussfit[i]-gtruecounts[i]),2);
            chi2bot += gaussfit[i];
        }
        chi2Final = chi2top/(4*chi2bot);
        gchi2Final = chi2Final;

        ui->RawCheck->setChecked(false);
        ui->BackgroundCheck->setChecked(false);
        ui->TrueCheck->setChecked(true);
        ui->ScatterCheck->setChecked(false);
        ui->LineCheck->setChecked(true);

        ui->customPlot->clearGraphs();
        EnergyPlot(gheader, {gtruecounts, gaussfit}, {"true", "gauss"}, "line");

    }
}

//Creating Background
void MainWindow::on_BackgroundSelectButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gbaseline == 0)
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gbaseline = 1;
        gfit = 0;
        ggaussian = 0;
        gsinglepeak = 0;
        gtwopeak = 0;
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaselineTracker.clear();
    }
}

//Creating a baseline
void MainWindow::on_BaselineButton_clicked()
{
    if(gbaselineTracker.isEmpty())
    {
        QMessageBox error_noback;
        error_noback.setText("No backgrounds selected. Please select background regions.");
        error_noback.exec();
    }
    else if(gbaselineTracker.size() % 2 != 0)
    {
        QMessageBox error_odd;
        error_odd.setText("Odd number of selections. Please make an even number of selections when defining background regions.");
        error_odd.exec();
    }
    else
    {
        QVector<double> x,y;
        double a,b;
        for(int i=0;i<(gbaselineTracker.size()/2);i++)
        {
            for(int j = gbaselineTracker[2*i];j <= gbaselineTracker[2*i+1];j++)
            {
                x.append(j);
            }
        }
        std::sort(x.begin(),x.end());

        for(int i=0;i<x.size();i++)
        {
            y.append(grawcounts[x[i]]);
        }
        // now we calculate all of the linear least squares sigma values from the formula
        double xsum=0,x2sum=0,ysum=0,xysum=0;                //variables for sums/sigma of xi,yi,xi^2,xiyi etc
        int n = x.size();
        for (int i=0;i<n;i++)
        {
            xsum=xsum+x[i];                        //calculate sigma(xi)
            ysum=ysum+y[i];                        //calculate sigma(yi)
            x2sum=x2sum+pow(x[i],2);                //calculate sigma(x^2i)
            xysum=xysum+x[i]*y[i];                    //calculate sigma(xi*yi)
        }
        a=(n*xysum-xsum*ysum)/(n*x2sum-xsum*xsum);            //calculate slope
        b=(x2sum*ysum-xsum*xysum)/(x2sum*n-xsum*xsum);            //calculate intercept
        QVector<double> y_fit;                       //an array to store the new fitted values of y
        for (int i=0;i<grawcounts.size();i++)
        {
            y_fit.append(a*i+b);                    //to calculate y(fitted) at given x points
        }
        gbackgroundcounts=y_fit;
        QVector<double> newtruecounts;
        for(int i=0;i<grawcounts.size();i++)
        {
            newtruecounts.append(grawcounts[i]-gbackgroundcounts[i]);
        }
        gtruecounts = newtruecounts;
        on_PlotButton_clicked();
    }
}

//Calibrate from bins to energy using a single peak
void MainWindow::on_SinglePeakCalibrateButton_clicked()
{
    QString energyperbin = ui->EnergyPerBinInput->text();
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(energyperbin.isEmpty())
    {
        QMessageBox error_notext;
        error_notext.setText("No energy per bin inputed. Please input an energy per bin or calibrate using two peaks.");
        error_notext.exec();
    }
    else if(gclickCounter != 1)
    {
        QMessageBox error_noclick;
        error_noclick.setText("Please ensure you have selected exactly one calibration line.");
        error_noclick.exec();
    }
    else if(gTheoEnergy.size() != 1)
    {
        QMessageBox error_noEnergy;
        error_noEnergy.setText("Please ensure you have exactly one selected item in the 'Energies' tab.");
        error_noEnergy.exec();
    }
    else
    {
        gEnergyPerBin = energyperbin.toDouble();
        gBinsPlot = 0;
        gtheoIgnore = 1;
        ui->customPlot->clearItems();
        on_PlotButton_clicked();
        QString message;
        message = QString("Energy Start = '%1' eV with counts '%'").arg(gEnergyVector[0]);
        ui->statusbar->showMessage(message, 10000);
    }
}

//select a fitting region for whole plot
void MainWindow::on_FitRegion_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gfit == 0)
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gbaseline = 0;
        ggaussian = 0;
        gsinglepeak = 0;
        gtwopeak = 0;
        gfit = 1;
        ggaussianTracker.clear();
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gsinglepeak = 0;
        gtwopeak = 0;
        gfitTracker.clear();
    }
}

//start selecting gaussian for gaussian fit
void MainWindow::on_GaussianButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(ggaussian == 0)
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 1;
        gfitTracker.clear();
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        ggaussianTracker.clear(); 
    }
}

//fit a single gaussian for initial estimate of LS fit.
void MainWindow::on_FitGaussianButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gEnergyVector.isEmpty())
    {
        QMessageBox error_nocal;
        error_nocal.setText("Not yet calibrated. Please calibrate before fitting.");
        error_nocal.exec();
    }
    if(ggaussianTracker.isEmpty())
    {
        QMessageBox error_nocal;
        error_nocal.setText("No Gausian Region selected. Please Mark Gaussian Region.");
        error_nocal.exec();
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        QVector<double> gaussfit, y;
        double sigma2 = 0, peakposition = 0;
        double maxheight = gtruecounts[ggaussianTracker[0]];
        int counter = abs(ggaussianTracker[1]-ggaussianTracker[0]);

        for(int i=ggaussianTracker[0];i<ggaussianTracker[1];i++)
        {
            y.append(gEnergyVector[i]);
        }
        double mu =0; //expected value
        for(int i=0;i<y.size();i++)
        {
            mu += y[i];
        }
        mu = mu/y.size();

        // find maxheight and peak position
        for(int i=ggaussianTracker[0];i<=ggaussianTracker[1];i++)
        {
            if(maxheight>=gtruecounts[i])
            {
                ;

            }
            else if(maxheight<gtruecounts[i])
            {
                maxheight = gtruecounts[i];
                peakposition = gEnergyVector[i];

            }
        }
        for(int i=0;i<y.size();i++)
        {
            sigma2 += pow(y[i]-mu,2);
        }
        sigma2 = sigma2/(y.size()-1); //variance

        //make the fit
        for(int i=0;i<gEnergyVector.size();i++)
        {
            gaussfit.append(maxheight*exp(-pow(gEnergyVector[i]-peakposition,2)/sigma2));
        }

        // finding chi squared
        double chi2top = 0, chi2bot = 0, chi2 = 0;
        for(int i=ggaussianTracker[0];i<ggaussianTracker[1];i++)
        {
            chi2top += pow(fabs(gtruecounts[i]-gaussfit[i]),2);
            chi2bot += gaussfit[i];
        }
        chi2 = chi2top/(4*chi2bot);

        //optimize sigma2
        double sigma2squeeze = sigma2, sigma2expand = sigma2, chi2squeeze = chi2, chi2expand = chi2;
        for(int i=1;i<1000;i++)
        {
            sigma2squeeze = sigma2 - i*sigma2/1000;
            for(int j=0;j<gEnergyVector.size();j++)
            {
                gaussfit[j]=(maxheight*exp(-pow(gEnergyVector[j]-peakposition,2)/sigma2squeeze));
            }

            double chi2top = 0, chi2bot = 0, chi2test = 0;
            for(int j=ggaussianTracker[0];j<ggaussianTracker[1];j++)
            {
                chi2top += pow(fabs(gtruecounts[j]-gaussfit[j]),2);
                chi2bot += gaussfit[j];
//                chi2top += pow(fabs(gtruecounts[i]-gaussfit[i]),2);
//                chi2bot += (y.size()-4)*(pow(y[i]-mu,2)/(y.size()-1));
            }
            chi2test = chi2top/(4*chi2bot);
            if(chi2test>=chi2squeeze || chi2test<1)
            {
                break;
            }
            if(chi2test<chi2squeeze)
            {
                chi2squeeze=chi2test;
            }
        }
        for(int i=1;i<1000;i++)
        {
            sigma2expand = sigma2 + i*sigma2/1000;
            for(int j=0;j<gEnergyVector.size();j++)
            {
                gaussfit[j]=(maxheight*exp(-pow(gEnergyVector[j]-peakposition,2)/sigma2expand));
            }

            double chi2top = 0, chi2bot = 0, chi2test = 0;
            for(int j=ggaussianTracker[0];j<ggaussianTracker[1];j++)
            {
                chi2top += pow(fabs(gtruecounts[j]-gaussfit[j]),2);
                chi2bot += gaussfit[j];
            }
            chi2test = chi2top/(4*chi2bot);
            if(chi2test>=chi2expand || chi2test<1)
            {
             break;
            }
            else if(chi2test<chi2expand)
            {
                chi2expand=chi2test;
            }
        }
        // now pick the best sigma2 based on the above
        if(chi2expand>chi2squeeze)
        {
            sigma2 = sigma2squeeze;
        }
        if(chi2expand<=chi2squeeze)
        {
            sigma2 = sigma2expand;
        }
        //keep it and plot it
        gsigma2 = sigma2;
        for(int i=0;i<gEnergyVector.size();i++)
        {
            gaussfit[i]=(maxheight*exp(-pow(gEnergyVector[i]-peakposition,2)/gsigma2));
        }

        ui->RawCheck->setChecked(false);
        ui->BackgroundCheck->setChecked(false);
        ui->TrueCheck->setChecked(true);
        ui->ScatterCheck->setChecked(false);
        ui->LineCheck->setChecked(true);

        ui->customPlot->clearGraphs();
        EnergyPlot(gheader, {gtruecounts, gaussfit}, {"true", "gauss"},"line");
    }
}

//start selecting region for twopeak calibration
void MainWindow::on_TwoPeakButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gtwopeak == 0)
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 0;
        gtwopeak = 1;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gclickCounter = 0;
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gclickCounter = 0;
    }
}

//start selecting peak for single peak calibration
void MainWindow::on_SinglePeakButton_clicked()
{
    if(gheader.isEmpty())
    {
        QMessageBox error_nodir;
        error_nodir.setText("No file selected. Please select a Directory and then select a file from the 'Files' tab.");
        error_nodir.exec();
    }
    else if(gsinglepeak == 0)
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 1;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gclickCounter = 0;
    }
    else
    {
        ui->customPlot->clearItems();
        ui->customPlot->replot();
        gsinglepeak = 0;
        gtwopeak = 0;
        gbaseline = 0;
        gfit = 0;
        ggaussian = 0;
        gclickCounter = 0;
    }
}

//zoom adjustments
void MainWindow::on_ReduceYButton_clicked()
{
    ui->customPlot->yAxis->scaleRange(.9);
    //set lower limit on y-axis
    ui->customPlot->yAxis->setRangeLower(-10);

    ui->customPlot->replot();
    gyScaleFactorDown += 1;
}

void MainWindow::on_IncreaseYButton_clicked()
{
    ui->customPlot->yAxis->scaleRange(1.1);
    //set lower limit on y-axis
    ui->customPlot->yAxis->setRangeLower(-10);
    ui->customPlot->replot();
    gyScaleFactorUp += 1;
}

void MainWindow::on_ReduceXButton_clicked()
{
    ui->customPlot->xAxis->scaleRange(.9);
    ui->customPlot->replot();
}

void MainWindow::on_IncreaseXButton_clicked()
{
    ui->customPlot->xAxis->scaleRange(1.1);
    ui->customPlot->replot();
}

//Export final data to CSV
void MainWindow::on_SaveButton_clicked()
{
    QString CSVdata = "Energy, Area, Height, Chi-Square, \n";
    int numrows = garea.size();

    for(int j=0;j<numrows; j++)
    {
        CSVdata.append(QString::number(gpeakposition[j]));
        CSVdata.append(", ");
        CSVdata.append(QString::number(garea[j]));
        CSVdata.append(", ");
        CSVdata.append(QString::number(gmaxheight[j]));
        CSVdata.append(", ");
        CSVdata.append(QString::number(gchi2[j]));
        CSVdata.append(" \n");
    }

    CSVdata.append("\n Reduced Chi-Square of entire fit = ");
    CSVdata.append(QString::number(gchi2Final));

    QString filename = QFileDialog::getSaveFileName(this, "DialogTitle", "filename.csv", "CSV files (.csv);;Zip files (.zip, *.7z)", 0, 0); // getting the filename (full path)
    QFile data(filename);
    if(data.open(QFile::WriteOnly |QFile::Truncate))
    {
        QTextStream output(&data);
        output << CSVdata;
    }
}

