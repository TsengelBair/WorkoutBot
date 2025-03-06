#ifndef CHART_H
#define CHART_H

#include <QMainWindow>
#include <qwt_plot.h>

class Chart : public QMainWindow
{
    Q_OBJECT

public:
    Chart(QWidget *parent = nullptr);
    ~Chart();

    void createPlot();

private:
    QwtPlot* _plot;
};
#endif // CHART_H
