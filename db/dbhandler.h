#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QSqlDatabase>
#include <QPair>

class DbHandler
{
private:
    DbHandler();
    ~DbHandler();

    QSqlDatabase db;
    static DbHandler* instance;

private:
    void openDb();
    bool createTables();
    int getExerciseId(const std::int64_t tg_id, const QString &exerciseName);
    int getOrInsertExerciseId(const std::int64_t tg_id, const QString &exerciseName);

public:
    static DbHandler* getInstance();
    QSqlDatabase& getDb();

    bool saveTrain(const std::int64_t tg_id, const QString& date, const QMap<QString, QList<double>> &trainInfo, QString& error);
    QMap<QString, double> trainData(const std::int64_t tg_id);
    QList<QPair<QString, double>> trainDataForExercise(const std::int64_t tg_id, QString &exerciseName, QString& error);
    QList<QString> getAllExercises(const std::int64_t tg_id, QString& errorStr);
};

#endif // DBHANDLER_H
