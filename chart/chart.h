#ifndef CHART_H
#define CHART_H

#include <QMainWindow>
#include <qwt_plot.h>

class Chart : public QMainWindow
{
    Q_OBJECT

public:
    /* передаем tg_id чата с пользователем, поскольку png изображения будут храниться под этим tg_id */
    Chart(std::int64_t id, QWidget *parent = nullptr);
    ~Chart();

    void createPlot(QMap<QString, double> &data);

private:
    std::int64_t _id;
    QwtPlot* _plot;
};
#endif // CHART_H
