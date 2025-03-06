#ifndef CHART_H
#define CHART_H

#include <QMainWindow>
#include <qwt_plot.h>

class Chart : public QMainWindow
{
    Q_OBJECT

public:
    Chart(std::int64_t id, QWidget *parent = nullptr);
    ~Chart();

    void createPlot();

private:
    std::int64_t _id;
    QwtPlot* _plot;
};
#endif // CHART_H
