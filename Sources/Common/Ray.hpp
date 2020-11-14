/**
 * Дополнение к математической библиотеке (Math.hpp). Класс луча, для нужд трассировки
 * Copyright (C) 2020 by Alex "DarkWolf" Nem - https://github.com/darkoffalex
 */
#pragma once

#include "Math.hpp"

namespace math
{
    class Ray
    {
    private:
        /// Начало луча
        math::Vec3<float> origin_;
        /// Направление луча
        math::Vec3<float> direction_;
        /// Вес луча
        float weight_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Ray() : origin_({}),direction_({0.0f,0.0f,-1.0f}),weight_(1.0f) {}

        /**
         * \brief Основной конструктор
         * \param origin Начало
         * \param direction Направление
         * \param weight Вес луча
         */
        Ray(const math::Vec3<float> &origin,const math::Vec3<float> &direction, const float &weight = 1.0f) :
                origin_(origin), direction_(math::Normalize(direction)), weight_(weight) {};

        /**
         * \brief Установить координаты начала луча
         * \param origin Точка в пространстве
         */
        void setOrigin(const math::Vec3<float>& origin){
            this->origin_ = origin;
        }

        /**
         * \brief Получить координаты начала луча
         * \return Точка в пространстве
         */
        const math::Vec3<float>& getOrigin() const {
            return this->origin_;
        }

        /**
         * \brief Установить вектор направления луча
         * \param direction Вектор в пространстве
         * \param normalize Нормализовать вектор
         */
        void setDirection(const math::Vec3<float>& direction, bool normalize = true){
            this->direction_ = normalize ? math::Normalize(direction) : direction;
        }

        /**
         * \brief Получить вектор направления луча
         * \return Вектор в пространстве
         */
        const math::Vec3<float>& getDirection() const {
            return this->direction_;
        }

        /**
         * \brief Установить вес (силу) луча
         * \param weight Значение веса
         */
        void setWeight(const float weight){
            this->weight_ = weight;
        }

        /**
         * \brief Получить вес (силу) луча
         * \return Значение веса
         */
        const float& getWeight() const {
            return weight_;
        }

        /**
         * \brief Пересечение со сферой
         * \param position Позиция центра сферы
         * \param radius Радиус сферы
         * \param tMin Минимальное расстояние до точки пересечения
         * \param tMax Максимальное расстояние до точки пересеченияя
         * \param tOut Расстояние от начала, до точки пересечения
         * \return Было ли пересечение с объектом
         */
        bool intersectsSphere(const math::Vec3<float>& position, float radius, float tMin, float tMax, float* tOut) const
        {
            // Уравнение сферы : (x−x0)^2+(y−y0)^2+(z−z0)^2 = R^2.
            // Уравнение сферы в векторном виде : |P−C|^2 = R^2 что эквивалентно dot((P−C),(P−C))=r2 (P - точка сфере, C - центр)
            // Параметрическое уравнение прямой (в векторном виде) : A+tB (А - начальная точка, B - направляющий вектор, t - параметр)
            // Подставив получим : dot((A+tB−C),(A+tB−C))=R^2
            // Преобразовав получим : t^2 * dot(B,B) + 2*dot(B,A−C) + dot(A−C,A−C) − r^2=0
            // Разделим на 3 части при коэффициенте t : a=dot(B,B) , b=2*dot(B,A−C), c=dot(A−C,A−C)−r2
            // Теперь можно решать квадратное уравнение относительно коэффициента t : t = (−b ± sqrt(b^2−4ac))/2a, где b2−4ac - дискриминант

            // Вектор к началу луча от центра сферы (для вычисления коэффициентов квадратного уравнения)
            math::Vec3<float> oc = this->origin_ - position;

            // Коэффициенты квадратного уравнения
            float a = math::Dot(this->direction_,this->direction_);
            float b = 2.0f * math::Dot(this->direction_,oc);
            float c = math::Dot(oc,oc) - (radius * radius);

            // Дискриминант
            float discriminant = b*b - 4.0f * a * c;

            // Если дискриминант отрицательный - пересечения не было
            if(discriminant < 0) return false;

            // 2 решения для параметра t
            float t1 = (-b - sqrtf(discriminant))/(2.0f * a);
            float t2 = (-b + sqrtf(discriminant))/(2.0f * a);

            // Нужна ближайшая точка пересечения, соотвественно t должен быть наименьший НО не отрицательный
            // Если оба значения t отрицательны, значит сфера сзади начальной точки луча, соответственно пересечния не было
            if(t1 < tMin && t2 < tMin) return false;

            // Если точка пересечения сзади, либо слишком близка, делаем t очень большим, чтобы при выборе наименьшего значения он не был выбрн
            if(t1 < tMin) t1 = tMax;
            if(t2 < tMin) t2 = tMax;
            auto tResult = std::min(t1,t2);

            if(tResult <= tMax){
                if(tOut != nullptr) *tOut = tResult;
                return true;
            }

            return false;
        }

        /**
         * \brief Пересечение с треугольником
         * \param v0 Координаты вершины 0
         * \param v1 Координаты вершины 1
         * \param v2 Координаты вершины 2
         * \param tMin Минимальное расстояние до точки пересечения
         * \param tMax Максимальное расстояние до точки пересеченияя
         * \param tOut Расстояние от начала, до точки пересечения
         * \param barycentricCoords Барицентрические координаты точки (используются для интерполяции)
         * \return Было ли пересечение с объектом
         */
        bool intersectsTriangle(
                const math::Vec3<float> &v0, const math::Vec3<float> &v1, const math::Vec3<float> &v2,
                float tMin, float tMax, float* tOut, math::Vec2<float>* barycentricCoords) const
        {
            // Два ребра треугольника
            auto e1 = v1 - v0;
            auto e2 = v2 - v0;

            // Нормаль треугольника (обход вершин по часовой стрелке)
            auto normal = math::Normalize(math::Cross(e2,e1));

            // Скалярное произведение нормали и направляющего вектора
            auto dot = math::Dot(this->direction_,normal);

            // Если луч не параллелен плоскости
            if(dot != 0)
            {
                // Вычисляем выраженный параметр t для параметрического уравнения прямой
                // Поскольку скалярно произведение нормали и луча (dot) совпадает со знаменателем выражения вычислять его повторно не нужно
                float t = (
                        (normal.x * v0.x - normal.x * this->origin_.x) +
                        (normal.y * v0.y - normal.y * this->origin_.y) +
                        (normal.z * v0.z - normal.z * this->origin_.z)) / dot;

                // Значение t (параметр уравнения) по сути представляет из себя длину вектора от начала луча, до пересечения
                // Если t отрицательно - луч направлен в обратную сторону от плоскости и пересечение не засчитывается
                // Пересечение также не считается засчитанным если дальность до точки вне требуемого диапозона
                if(t > 0 && t >= tMin && t <= tMax)
                {
                    // Точка пересечения
                    auto intersectionPoint = this->origin_ + (this->direction_ * t);

                    // Далее нужно определить находится ли точка нутри треугольника а также получить ее
                    // барицентрические координаты для дальнейшего вычисления интерполированных значений
                    // Для этого нужно использовать ребра треугольника как базисные вектора нового пространства
                    // и оценить где в этом пространстве находится точка

                    // Поскольку базсные вектора смещены на чало координат с точкой следует сделать то же самое
                    auto pt = intersectionPoint - v0;
                    // Матрица 3*3 описывающая пространство треугольника
                    math::Mat3<float> triangleSpace = math::Inverse(math::Mat3<float>(e2,e1,normal));
                    // Получаем точку в пространстве треугольника
                    auto result = triangleSpace * pt;

                    // Если точка находится внутри треугольника
                    if(result.x >= 0.0f && result.y >= 0.0f && result.x + result.y <= 1.0f)
                    {
                        // Отдать барицентрические координаты
                        (*barycentricCoords).x = result.x;
                        (*barycentricCoords).y = result.y;

                        // Отдать расстояние до пересечения
                        *tOut = t;

                        return true;
                    }
                }
            }

            return false;
        }

        /**
         * \brief Пересечение с плоскостью
         * \param normal Нормаль плоскости
         * \param p0 Точка на плоскости
         * \param tMin Минимальное расстояние до точки пересечения
         * \param tMax Максимальное расстояние до точки пересеченияя
         * \param tOut Расстояние от начала, до точки пересечения
         * \return Было ли пересечение с объектом
         */
        bool intersectsPlane(const math::Vec3<float>& normal, const math::Vec3<float> p0, float tMin, float tMax, float* tOut) const
        {
            // Скалярное произведение нормали и направляющего вектора
            auto dot = math::Dot(this->direction_,normal);

            // Если луч не параллелен плоскости
            if(dot != 0)
            {
                // Вычисляем выраженный параметр t для параметрического уравнения прямой
                // Поскольку скалярно произведение нормали и луча (dot) совпадает со знаменателем выражения вычислять его повторно не нужно
                float t = (
                        (normal.x * p0.x - normal.x * this->origin_.x) +
                        (normal.y * p0.y - normal.y * this->origin_.y) +
                        (normal.z * p0.z - normal.z * this->origin_.z)) / dot;

                // Значение t (параметр уравнения) по сути представляет из себя длину вектора от начала луча, до пересечения
                // Если t отрицательно - луч направлен в обратную сторону от плоскости и пересечение не засчитывается
                // Пересечение также не считается засчитанным если дальность до точки вне требуемого диапозона
                if(t > 0 && t >= tMin && t <= tMax)
                {
                    *tOut = t;
                    return true;
                }
            }

            return false;
        }
    };
}