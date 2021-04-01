#pragma once

#include <ctime>
#include <chrono>
#include <random>
#include <memory>

#include <Math.hpp>
#include <Ray.hpp>

/**
 * \brief Случайное float значение
 * \param min Минимальная граница
 * \param max Максимальная границе
 * \return Случайное значение
 */
inline float RndFloat(float min = 0.0f, float max = 1.0f){
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::default_random_engine generator(static_cast<size_t>(ms.count()));
    std::uniform_real_distribution<float> distribution(min, max);

    return distribution(generator);
}

/**
 * \brief Случайный вектор в заданых пределах
 * \param min Минимальное значение всех координат
 * \param max Максимальное значение всех координат
 * \return Вектор
 */
inline math::Vec3<float> RndVec(float min = -1.0f, float max = 1.0f){
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::default_random_engine generator(static_cast<size_t>(ms.count()));
    std::uniform_real_distribution<float> distribution(min, max);

    return {
            distribution(generator),
            distribution(generator),
            distribution(generator)
    };
}

/**
 * \brief Случайная точка в пределах единичной сферы
 * \return Вектор
 */
inline math::Vec3<float> RndUnitSpherePoint(){
    while (true){
        auto p = RndVec();
        if(math::LengthSquared(p) >= 1) continue;
        return p;
    }
}

/**
 * \brief Случайный вектор в пределах полусферы в направлении dir
 * \param dir Направление (ориентация) полусферы
 * \param thetaMax Максимальное отклонение направления (90 для полной полусферы)
 * \return Вектор
 */
inline math::Vec3<float> RndHemisphereVec(const math::Vec3<float>& dir, const float& thetaMax = 90.0f)
{
    // Генератор случайных чисел
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::default_random_engine generator(static_cast<size_t>(ms.count()));

    // Коэффициенты распределения для двух уголов
    std::uniform_real_distribution<float> fiDist(0.0f,1.0f);
    std::uniform_real_distribution<float> thetaDist(0.0f, 1.0f);

    // Вектор перпендикулярный вектору направления
    auto b = math::Normalize(math::Cross(dir, dir + math::Vec3<float>(0.01f, 0.01f, 0.01f)));

    // Второй перпендикулярный вектор
    auto c = math::Normalize(math::Cross(dir, b));

    // Случайные углы (fi - для вектора вращяющегося вокруг направления dir, theta - угол между итоговым вектором и направлением dir)
    float fi = (fiDist(generator) * 360.0f) / 57.2958f; // [0 - 360]
    float theta = (thetaDist(generator) * thetaMax) / 57.2958f; // [0 - 90]

    // Вектор описывающий круг вокруг направления dir
    math::Vec3<float> d = (b * std::cos(fi)) + (c * std::sin(fi));
    // Вектор отклоненный от dir на случайный угол theta
    return (dir * std::cos(theta)) + (d * std::sin(theta));
}

/**
 * \brief Случайный вектор в пределах полусферы в направлении dir (реализация через сферические координаты)
 * \param dir Направление (ориентация) полусферы
 * \param thetaMax Максимальное отклонение направления (90 для полной полусферы)
 * \return Вектор
 */
inline math::Vec3<float> RndHemisphereVec2(const math::Vec3<float>& dir, const float& thetaMax = 90.0f)
{
    // Вектор перпендикулярный вектору направления
    auto b = math::Normalize(math::Cross(dir, dir + math::Vec3<float>(0.01f, 0.01f, 0.01f)));
    // Второй перпендикулярный вектор
    auto c = math::Normalize(math::Cross(dir, b));
    // Матрица для перевода из координат пространства направления (dir) в мировые координаты
    // В локальном пространстве вектор направления является вектором вверх, и ссответствует оси Y
    auto dirSpaceToWorldSpace = math::Mat3<float>(b,dir,c);

    // Коэффициенты распределения для двух уголов
    std::uniform_real_distribution<float> fiDist(0.0f,1.0f);
    std::uniform_real_distribution<float> thetaDist(0.0f, 1.0f);

    // Генератор случайных чисел
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::default_random_engine generator(static_cast<size_t>(ms.count()));

    // Случайные углы (fi - азимутный угол, theta - полярный угол)
    float fi = (fiDist(generator) * 360.0f) / 57.2958f; // [0 - 360]
    float theta = (thetaDist(generator) * thetaMax) / 57.2958f; // [0 - 90]

    // Перевод из сферичесих координат в декартовы, получение вектора в локальном пространстве
    math::Vec3<float> dirLocal = {
            std::sin(theta) * std::cos(fi),
            std::cos(theta),
            std::sin(theta) * std::sin(fi)
    };

    // Вектор направления в мировых координатах
    return dirSpaceToWorldSpace * dirLocal;
}

/**
 * \brief Случайный вектор в пределах полусферы в направлении dir (более равномерное распределения засчет выборки по высоте)
 * \param dir Направление (ориентация) полусферы
 * \param thetaMax Максимальное отклонение направления (90 для полной полусферы)
 * \return Вектор
 */
inline math::Vec3<float> RndHemisphereVec3(const math::Vec3<float>& dir, const float& thetaMax = 90.0f)
{
    // Генератор случайных чисел
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::default_random_engine generator(static_cast<size_t>(ms.count()));

    // Коэффициенты распределения для двух угло
    // Второй угол вычисляется через случайную высоту, это дает наиболее равномерное распределение по полусфере
    std::uniform_real_distribution<float> fiDist(0.0f,1.0f);
    std::uniform_real_distribution<float> heightDist(std::cos(thetaMax/57.2958f), 1.0f);

    // Вектор перпендикулярный вектору направления
    auto b = math::Normalize(math::Cross(dir, dir + math::Vec3<float>(0.01f, 0.01f, 0.01f)));

    // Второй перпендикулярный вектор
    auto c = math::Normalize(math::Cross(dir, b));

    // Случайные углы (fi - для вектора вращяющегося вокруг направления dir, theta - угол между итоговым вектором и направлением dir)
    float fi = (fiDist(generator) * 360.0f) / 57.2958f; // [0 - 360]
    float theta = std::acos(heightDist(generator)); //[0 - 90]

    // Вектор описывающий круг вокруг направления dir
    math::Vec3<float> d = (b * std::cos(fi)) + (c * std::sin(fi));
    // Вектор отклоненный от dir на случайный угол theta
    return (dir * std::cos(theta)) + (d * std::sin(theta));
}

/**
 * \brief Предварительная декларация материала
 */
namespace materials
{
    class Material;
}


/**
 * \brief Информация о точке пересечения луча и объекта
 */
struct HitInfo
{
    /// Положение точки
    math::Vec3<float> point = {};
    /// Нормаль
    math::Vec3<float> normal = {};
    /// Дистанция до точки пересеенич
    float t = 0.0f;
    /// Является ли сторона "лицевой"
    bool frontFaceSurface = true;
    /// Указатель на материал в точке пересечения
    std::shared_ptr<materials::Material> materialPtr = nullptr;
};

/**
 * \brief Базовые классы для описания материалов
 */
namespace materials
{
    /**
     * \brief Базовый абстрактный класс материала
     *
     * \details Материал описывает как переотражаются лучи, разброс лучей, потрею света (затухание) после отражений.
     * Некоторые материалы вовсе не переотражают лучи, при этом могут сами излучать свет
     */
    class Material
    {
    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Material() = default;

        /**
         * \brief Деструктор по умолчанию
         */
        virtual ~Material() = default;

        /**
         * \brief Происходит ли переотражение разброс лучей для данного материала
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        virtual bool isScatters(const math::Ray& rayIn, const HitInfo& hitInfo) const = 0;

        /**
         * \brief Излучает ли данный материал свет
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \return Да или нет
         */
        virtual bool isEmits(const math::Ray& rayIn, const HitInfo& hitInfo) const = 0;

        /**
         * \brief Получить переотраженный-разбросанный луч
         * \param rayIn Входной луч
         * \param hitInfo Информация о пересечении с поверхностью
         * \param attenuationOut Затухание для переотраженного луча
         * \return Переотраженный луч
         */
        virtual math::Ray scatteredRay(const math::Ray& rayIn, const HitInfo& hitInfo, math::Vec3<float>* attenuationOut) const = 0;

        /**
         * \brief Излученный цвет
         * \return Цветовой вектор
         */
        virtual math::Vec3<float> emittedColor() const = 0;
    };
}

/**
 * \brief Базовые классы для описания сцены
 */
namespace scene
{
    /**
     * \brief Базовый абстрактный класс для элементов сцены
     */
    class Hittable
    {
    protected:
        /// Материал элемента сцены
        std::shared_ptr<materials::Material> materialPtr_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        Hittable():materialPtr_(nullptr){}

        /**
         * \brief Основной коструктор
         * \param materialPtr Указатель на метериал
         */
        explicit Hittable(std::shared_ptr<materials::Material> materialPtr):materialPtr_(std::move(materialPtr)){}

        /**
         * \brief Деструктор
         */
        virtual ~Hittable() = default;

        /**
         * \brief Установить материал
         * \param materialPtr Указатель на метериал
         */
        void setMaterial(const std::shared_ptr<materials::Material>& materialPtr){
            this->materialPtr_ = materialPtr;
        }

        /**
         * \brief Получить материал
         * \return Указатель на метериал
         */
        const std::shared_ptr<materials::Material>& getMaterial() const{
            return materialPtr_;
        }

        /**
         * \brief Пересечение объекта сцены и луча (полностью виртуальный метод)
         * \param ray Луч
         * \param tMin Минимальное расстояние
         * \param tMax Максимальное расстояние
         * \param hitInfo Информация о пересечении
         * \return Было ли пересечение с объектом
         */
        virtual bool intersectsRay(const math::Ray& ray, float tMin, float tMax, HitInfo* hitInfo) const = 0;
    };

    /**
     * \brief Список элементов сцены
     *
     * \details Наследуется от абстрактного класса Hittable и реализует метод intersectsRay, поэтому может
     * считаться полноправным элементом сцены (как и другие потомки Hittable)
     */
    class List : public Hittable
    {
    private:
        /// Массив элементов сцены
        std::vector<std::shared_ptr<Hittable>> elements_;

    public:
        /**
         * \brief Конструктор по умолчанию
         */
        List():Hittable(){}

        /**
         * \brief Основной конструктор
         * \param firstElement Первый элемент списка
         */
        explicit List(const std::shared_ptr<Hittable>& firstElement)
        {
            this->elements_.push_back(firstElement);
        }

        /**
         * \brief Получить массив элементов
         * \return Константная ссылка на массив элементов
         */
        const std::vector<std::shared_ptr<Hittable>>& getElements() const
        {
            return elements_;
        }

        /**
         * \brief Добавить элемент
         * \param element Указатель на элемент сцены
         */
        void addElement(const std::shared_ptr<Hittable>& element)
        {
            this->elements_.push_back(element);
        }

        /**
         * \brief Очистка массива элементов
         */
        void clear()
        {
            this->elements_.clear();
            this->elements_.shrink_to_fit();
        }

        /**
         * \brief Пересечение всех объектов сцены и луча
         * \param ray Луч
         * \param tMin Минимальное расстояние
         * \param tMax Максимальное расстояние
         * \param hitInfo Информация о пересечении
         * \return Было ли пересечение с объектом
         */
        bool intersectsRay(const math::Ray& ray, float tMin, float tMax, HitInfo* hitInfo) const override
        {
            // Информация о пересечении
            HitInfo hit{};
            // Было ли пересечение с каким-либо объектом
            bool hitAnything = false;
            // Расстояние до ближ. пересечения
            float closest = tMax;

            // Проход по всем элементам
            for(const auto& element : elements_)
            {
                // При каждом пересечении максимальное расстояние (tMax) ограничивается расстоянием текущего пересечения
                // Таким образом гарантируется нахождение ближайшей точки пересечения (слишком далекие исключаются лимитом)

                // Если было пересечение элемента и луча
                if(element->intersectsRay(ray,tMin,closest,&hit))
                {
                    // Пересечение засчитано
                    hitAnything = true;
                    // Уменьшить расстояние
                    closest = hit.t;
                    // Информация о пересечении
                    *hitInfo = hit;
                }
            }

            return hitAnything;
        }
    };
}