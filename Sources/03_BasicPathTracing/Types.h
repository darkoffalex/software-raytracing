#pragma once

#include <vector>
#include <memory>
#include <limits>

#include <Math.hpp>
#include <Ray.hpp>

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
};

/**
 * \brief Трассируемый лучами элемент
 */
class HittableElement
{
public:
    /**
     * \brief Конструктор по умолчанию
     */
    HittableElement() = default;

    /**
     * \brief Деструктор
     */
    virtual ~HittableElement() = default;

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