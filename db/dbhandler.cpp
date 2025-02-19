#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QDate>
#include "dbhandler.h"

DbHandler* DbHandler::instance = nullptr;

DbHandler::DbHandler()
{
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("trains_db");
    db.setUserName("postgres");
    openDb();
}

DbHandler::~DbHandler()
{
    if (db.isOpen()){
        db.close();
    }
}

DbHandler *DbHandler::getInstance()
{
    if (!instance){
        instance = new DbHandler();
    }

    return instance;
}

void DbHandler::openDb()
{
    if (db.open()){
        qDebug() << "Успешное подключение к БД";
    } else {
        qDebug() << "Ошибка подключения к БД" << db.lastError().text();
    }
}

/* Функция для получения ID упражнения (либо INSERT либо возврат уже имеющегося)*/
int DbHandler::getOrInsertExerciseId(const QString &exerciseName) {
    QSqlQuery query;

    query.prepare("INSERT INTO exercises (exercise_name) VALUES (:exerciseName) ON CONFLICT (exercise_name) DO NOTHING");
    query.bindValue(":exerciseName", exerciseName);

    if (!query.exec()) {
        qDebug() << "Ошибка при insert в таблицу exercises:" << query.lastError();
        return -1;
    }

    query.prepare("SELECT id FROM exercises WHERE exercise_name = :exerciseName");
    query.bindValue(":exerciseName", exerciseName);

    if (!query.exec()) {
        qDebug() << "Ошибка при select из таблицы exercises:" << query.lastError();
        return -1;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

void DbHandler::saveTrain(const QString &date, const QMap<QString, QList<double> > &trainInfo)
{
    QSqlQuery query;
    query.prepare("INSERT INTO workouts (workout_date) VALUES (:date) RETURNING id");
    query.bindValue(":date", QDate::fromString(date, "dd-MM-yyyy"));

    if (!query.exec()){
        qDebug() << "Ошибка при insert в основную таблицу";
    }

    query.next(); /// переходим к рез-ту запроса
    int workoutId = query.value(0).toInt();

    /// INSERT в таблицу с упражнениями, получаем id упражнения и вып-ем в цикле insertы в таблицу sets
    for (const auto &exercise : trainInfo.keys()) {
        int exerciseId = getOrInsertExerciseId(exercise);
        if (exerciseId == -1) {
            // Обработка ошибки
            qDebug() << "Ошибка с id упражнения";
            continue;
        }

        /// Вставляем данные в таблицу sets
        for (double tonnage : trainInfo[exercise]) {
            QSqlQuery setQuery;
            setQuery.prepare("INSERT INTO sets (workout_id, exercise_id, tonnage) VALUES (:workoutId, :exerciseId, :tonnage)");
            setQuery.bindValue(":workoutId", workoutId);
            setQuery.bindValue(":exerciseId", exerciseId);
            setQuery.bindValue(":tonnage", tonnage);

            if (!setQuery.exec()) {
                qDebug() << "Ошибка при insert в таблицу sets:" << setQuery.lastError();
            }
        }
    }
}

QSqlDatabase &DbHandler::getDb()
{
    if (!instance){
        throw std::runtime_error("Db is not open!");
    }

    return db;
}
