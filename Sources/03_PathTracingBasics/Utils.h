#pragma once

#include <utility>
#include <vector>
#include <memory>
#include <limits>
#include <ctime>
#include <chrono>
#include <random>

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
    static std::mt19937 generator(ms.count());
    static std::uniform_real_distribution<float> distribution(min, max);

    return distribution(generator);
}

/**
 * \brief Случайный вектор в заданых пределах
 * \param min Минимальное значение всех координат
 * \param max Максимальное значение всех координат
 * \return Вектор
 */
inline math::Vec3<float> RndVec3(float min = -1.0f, float max = 1.0f){
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::mt19937 generator(ms.count());
    static std::uniform_real_distribution<float> distribution(min, max);

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
        auto p = RndVec3();
        if(math::LengthSquared(p) >= 1) continue;
        return p;
    }
}

/**
 * \brief Случайный вектор в пределах полусферы направленной в направлении a
 * \param dir Направление (ориентация) полусферы
 * \param thetaMax Максимальное отклонение направления (90 для полной полусферы)
 * \return Вектор
 */
inline math::Vec3<float> RndHemisphereVec(const math::Vec3<float>& dir, const float& thetaMax = 90.0f)
{
    // Генератор случайных чисел
    static std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    static std::mt19937 generator(ms.count());

    // Коэффициенты распределения для двух уголов
    std::uniform_real_distribution<float> fiDist(0.0f,1.0f);
    std::uniform_real_distribution<float> thetaDist(-1.0f, 1.0f);

    // Вектор перпендикулярный вектору направления
    auto b = math::Normalize(math::Cross(dir, dir + math::Vec3<float>(0.01f, 0.01f, 0.01f)));

    // Второй перпендикулярный вектор
    auto c = math::Normalize(math::Cross(dir, b));

    // Случайные углы (fi - для вектора вращяющегося вокруг направления dir, fi - угол между итоговым вектором и направлением dir)
    float fi = (fiDist(generator) * 360.0f) / 57.2958f; // [0 - 360]
    float theta = (thetaDist(generator) * thetaMax) / 57.2958f; // [-90 - 90]

    // Вектор описывающий круг вокруг направления dir
    math::Vec3<float> d = (b * std::cos(fi)) + (c * std::sin(fi));
    // Вектор отклоненный от dir на случайный угол theta
    return (dir * std::cos(theta)) + (d * std::sin(theta));
}

/**
 * \brief Предварительная декларация материала
 */
class Material;

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
    std::shared_ptr<Material> materialPtr = nullptr;
};

/**
 * \brief Абстрактный класс материала
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
     * \brief Полностью виртуальный метод разброса луча
     * \param rayIn Входной луч
     * \param hitInfo Информация о пересечении
     * \param attenuationOut Затенение цвета для переотраженного луча
     * \param scatteredRayOut Переотраженный луч
     * \return Было ли переотражение
     */
    virtual bool scatter(
            const math::Ray& rayIn,
            const HitInfo& hitInfo,
            math::Vec3<float>* attenuationOut,
            math::Ray* scatteredRayOut) const = 0;
};

/**
 * \brief Трассируемый лучами элемент
 */
class HittableElement
{
protected:
    /// Материал элемента сцены
    std::shared_ptr<Material> materialPtr_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    HittableElement():materialPtr_(nullptr){}

    /**
     * \brief Основной коструктор
     * \param materialPtr Указатель на метериал
     */
    explicit HittableElement(std::shared_ptr<Material> materialPtr):materialPtr_(std::move(materialPtr)){}

    /**
     * \brief Деструктор
     */
    virtual ~HittableElement() = default;

    /**
     * \brief Установить материал
     * \param materialPtr Указатель на метериал
     */
    void setMaterial(const std::shared_ptr<Material>& materialPtr){
        this->materialPtr_ = materialPtr;
    }

    /**
     * \brief Получить материал
     * \return Указатель на метериал
     */
    const std::shared_ptr<Material>& getMaterial() const{
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
 * \brief Сцена (контейнер элементов)
 */
class Scene : public HittableElement
{
private:
    /// Массив элементов сцены
    std::vector<std::shared_ptr<HittableElement>> elements_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    Scene():HittableElement(){}

    /**
     * \brief Основной конструктор
     * \param element Указатель на элемент сцены
     */
    explicit Scene(const std::shared_ptr<HittableElement>& element)
    {
        this->addElement(element);
    }

    /**
     * \brief Получить массив элементов
     * \return Константная ссылка на массив элементов
     */
    const std::vector<std::shared_ptr<HittableElement>>& getElements() const
    {
        return elements_;
    }

    /**
     * \brief Добавить элемент
     * \param element Указатель на элемент сцены
     */
    void addElement(const std::shared_ptr<HittableElement>& element)
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
        HitInfo hit{};
        bool hitAnything = false;
        float closest = tMax;

        for(const auto& element : elements_)
        {
            if(element->intersectsRay(ray,tMin,closest,&hit)){
                hitAnything = true;
                closest = hit.t;
                *hitInfo = hit;
            }
        }

        return hitAnything;
    }
};