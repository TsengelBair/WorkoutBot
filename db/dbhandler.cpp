#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QDate>
#include "dbhandler.h"

DbHandler* DbHandler::instance = nullptr;

DbHandler::DbHandler()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("trains_sqlite.db");
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
        bool ok = createTables();
        if (ok) qDebug() << "Таблицы созданы успешно";
    } else {
        qDebug() << "Ошибка подключения к БД" << db.lastError().text();
    }
}

bool DbHandler::createTables()
{
    QSqlQuery query;

    if (!query.exec("CREATE TABLE IF NOT EXISTS workouts ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "tg_id INTEGER NOT NULL, "
                    "workout_date DATE NOT NULL, "
                    "UNIQUE(tg_id, workout_date))")) { /// ограничение уникальности, только одна тренировка за день
        qDebug() << "Ошибка при создании таблицы workouts:" << query.lastError();
        return 0;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS exercises ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "tg_id INTEGER NOT NULL, "
                        "exercise_name TEXT NOT NULL, "
                        "UNIQUE(tg_id, exercise_name))")) {
        qDebug() << "Ошибка при создании таблицы exercises:" << query.lastError();
        return 0;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS sets ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "tg_id INTEGER NOT NULL, "
                        "workout_id INTEGER NOT NULL, "
                        "exercise_id INTEGER NOT NULL, "
                        "tonnage REAL NOT NULL, "
                        "FOREIGN KEY (workout_id) REFERENCES workouts(id) ON DELETE CASCADE, "
                        "FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE CASCADE)")) {
        qDebug() << "Ошибка при создании таблицы sets:" << query.lastError();
        return 0;
    }

    return 1;
}

QList<QPair<QString, double>> DbHandler::trainDataForExercise(const int64_t tg_id, QString &exerciseName, QString& error)
{
    QList<QPair<QString, double>> data;

    int exerciseID = getExerciseId(tg_id, exerciseName);
    if (exerciseID == -1) {
        error = "Not exist";
        return data;
    }

    QSqlQuery query;
    query.prepare("SELECT w.workout_date, SUM(s.tonnage) AS total_tonnage "
                  "FROM sets s JOIN workouts w ON s.workout_id = w.id "
                  "WHERE s.exercise_id = :exerciseID "
                  "GROUP BY w.workout_date ORDER BY w.workout_date");

    query.bindValue(":exerciseID", exerciseID);

    if (!query.exec()) {
        error = "Error";
        return data;
    }

    while (query.next()) {
        QString workoutDate = query.value(0).toString();
        double totalTonnage = query.value(1).toDouble();
        qDebug() << totalTonnage;

        data.push_back(qMakePair(workoutDate, totalTonnage));
    }

    return data;
}


int DbHandler::getExerciseId(const int64_t tg_id, const QString &exerciseName)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM exercises WHERE tg_id = :tg_id AND exercise_name = :exerciseName");

    query.bindValue(":tg_id", QVariant::fromValue(tg_id));
    query.bindValue(":exerciseName", exerciseName);

    if (!query.exec()) {
        qDebug() << "Ошибка при select из таблицы exercises:" << query.lastError();
        return -1;
    }
    if (query.next()) {
        return query.value(0).toInt(); /// Возвращаем ID упражнения
    }

    return -1; /// ошибка
}


/* Функция для получения ID упражнения (либо INSERT либо возврат уже имеющегося) */
int DbHandler::getOrInsertExerciseId(const std::int64_t tg_id, const QString &exerciseName) {
    QSqlQuery query;

    query.prepare("INSERT INTO exercises (tg_id, exercise_name) VALUES (:tg_id, :exerciseName)");
    query.bindValue(":tg_id", QVariant::fromValue(tg_id));
    query.bindValue(":exerciseName", exerciseName);

    if (!query.exec()) {
        /// Если ошибка, проверяем, не существует ли уже такое упражнение
        if (query.lastError().text().contains("UNIQUE constraint failed")) {
            return getExerciseId(tg_id, exerciseName); /// Возвращаем ID существующего упражнения
        } else {
            qDebug() << "Ошибка при insert в таблицу exercises:" << query.lastError();
            return -1;
        }
    }

    /// Если вставка прошла успешно, получаем ID нового упражнения
    return query.lastInsertId().toInt();
}

bool DbHandler::saveTrain(const std::int64_t tg_id, const QString &date, const QMap<QString, QList<double>> &trainInfo, QString& error)
{
    QSqlQuery query;
    query.prepare("INSERT INTO workouts (tg_id, workout_date) VALUES (:tg_id, :date)");
    query.bindValue(":tg_id", QVariant::fromValue(tg_id));
    query.bindValue(":date", QDate::fromString(date, "dd-MM-yyyy"));

    if (!query.exec()){
        error = query.lastError().text();
        qDebug() << "Ошибка при insert в основную таблицу:" << error;
        return false;
    }

    /// Получаем ID последней вставленной записи по tg_id и workout_date
    query.prepare("SELECT id FROM workouts WHERE tg_id = :tg_id AND workout_date = :date");
    query.bindValue(":tg_id", QVariant::fromValue(tg_id));
    query.bindValue(":date", QDate::fromString(date, "dd-MM-yyyy"));

    if (!query.exec()) {
        qDebug() << "Ошибка при select из таблицы workouts:" << query.lastError();
        return false;
    }

    int workoutId = -1; /// Инициализируем переменную для ID тренировки
    if (query.next()) {
        workoutId = query.value(0).toInt(); /// Получаем ID последней вставленной записи
    } else {
        qDebug() << "Не удалось получить ID последней вставленной записи";
        return false;
    }

    /// INSERT в таблицу с упражнениями, получаем id упражнения и выполним в цикле insertы в таблицу sets
    for (const auto &exercise : trainInfo.keys()) {
        int exerciseId = getOrInsertExerciseId(tg_id, exercise);
        if (exerciseId == -1) {
            return false;
        }

        /// Вставляем данные в таблицу sets
        for (double tonnage : trainInfo[exercise]) {
            QSqlQuery setQuery;
            setQuery.prepare("INSERT INTO sets (tg_id, workout_id, exercise_id, tonnage) VALUES (:tg_id, :workoutId, :exerciseId, :tonnage)");
            setQuery.bindValue(":tg_id", QVariant::fromValue(tg_id));
            setQuery.bindValue(":workoutId", workoutId);
            setQuery.bindValue(":exerciseId", exerciseId);
            setQuery.bindValue(":tonnage", tonnage);

            if (!setQuery.exec()) {
                qDebug() << "Ошибка при insert в таблицу sets:" << setQuery.lastError();
                return false;
            }
        }
    }

    return true;
}

QMap<QString, double> DbHandler::trainData(const std::int64_t tg_id)
{
    QMap<QString, double> data;

    QSqlQuery query;
    QString queryStr = "SELECT workouts.workout_date, SUM(sets.tonnage) AS total_tonnage "
                       "FROM workouts "
                       "JOIN sets ON workouts.id = sets.workout_id "
                       "WHERE workouts.tg_id = :tg_id "
                       "GROUP BY workouts.workout_date "
                       "ORDER BY workouts.workout_date;";

    query.prepare(queryStr);
    query.bindValue(":tg_id", QVariant::fromValue(tg_id));

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        QString workoutDate = query.value(0).toString();
        double totalTonnage = query.value(1).toDouble();

        data.insert(workoutDate, totalTonnage);
    }

    return data;
}

QList<QString> DbHandler::getAllExercises(const int64_t tg_id, QString& errorStr)
{
    QList<QString> exercises;

    QSqlQuery query;
    QString queryStr = "SELECT * FROM exercises WHERE tg_id = :tg_id";

    query.prepare(queryStr);
    query.bindValue(":tg_id", QVariant::fromValue(tg_id));

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        errorStr = "Error";
        return exercises;
    }

    while (query.next()) {
        QString exercise = query.value(2).toString();
        exercises.push_back(exercise);
    }

    if (exercises.isEmpty()) {
        errorStr = "No data";
    }

    return exercises;
}

QSqlDatabase &DbHandler::getDb()
{
    if (!instance){
        throw std::runtime_error("Db is not open!");
    }

    return db;
}
