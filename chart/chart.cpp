#include <qwt_date_scale_engine.h>
#include <qwt_date_scale_draw.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <QTimer>
#include <QDir>

#include "chart.h"
#include "db/dbhandler.h"

Chart::Chart(std::int64_t id, QWidget *parent)
    : QMainWindow(parent),
      _id(id)
{
    _plot = new QwtPlot(this);
    setCentralWidget(_plot);
}

Chart::~Chart()
{
}

void Chart::createPlot()
{
    /// Установка осей для отображения даты
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw);
    _plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtDateScaleEngine);

    // Создание кривой
    QwtPlotCurve *curve = new QwtPlotCurve();
    curve->setPen(Qt::blue, 2);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse, QBrush(Qt::yellow), QPen(Qt::red, 2), QSize(8, 8));
    curve->setSymbol(symbol);

    QMap<QString, double> data = DbHandler::getInstance()->trainData(_id);

    /// Использование QVector для хранения точек
    QVector<QPointF> points;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        QString dateString = it.key();
        double tonnage = it.value();

        QDateTime dateTime = QDateTime::fromString(dateString, "yyyy-MM-dd");

        /// Проверка на валидность даты
        if (dateTime.isValid()) {
            QPointF point = QPointF(QwtDate::toDouble(dateTime), tonnage);
            points.append(point);
        } else {
            qDebug() << "Invalid date:" << dateString;
        }
    }

    /// Передаем вектор точек в кривую
    curve->setSamples(points);
    curve->attach(_plot);

    _plot->replot();

    QDir dir("charts");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QPixmap pixmap = _plot->grab();
    QString path = "charts/" + QString::number(_id) + ".png";
    if (pixmap.save(path)) {
        qDebug() << "OK";
    } else {
        qDebug() << "Error";
    }
}
