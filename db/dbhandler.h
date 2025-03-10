#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QSqlDatabase>

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
    int getOrInsertExerciseId(const std::int64_t tg_id, const QString &exerciseName);

public:
    static DbHandler* getInstance();
    QSqlDatabase& getDb();

    bool saveTrain(const std::int64_t tg_id, const QString& date, const QMap<QString, QList<double>> &trainInfo);
    QMap<QString, double> trainData(const std::int64_t tg_id);
};

#endif // DBHANDLER_H
