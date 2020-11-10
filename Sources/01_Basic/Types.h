#pragma once

#include <Math.hpp>
#include <Ray.hpp>

/**
 * \brief Материал объекта сцены (освещение по Фонгу)
 */
struct Material
{
    /// Собственный цвет поверхности
    math::Vec3<float> albedo = {1.0f,1.0f,1.0f};
    /// Бликовость
    float specularIntensity = 1.0f;
    /// Бликовая экспонента
    float shininess = 16.0f;
    /// Коэффициент отношения основных компонентов освещенности (диффузный, бликовый) и второстепелнных компонентов (отражение, преломление)
    float primaryToSecondary = 1.0f;
    /// Коэффициент отношения отражения к преломлению
    float reflectToRefract = 1.0f;
    /// Коэффициент преломления
    float refractionEta = 1.0f;
};

/**
 * \brief Источник света
 */
struct LightSource
{
    /// Положение источника света в пространстве
    math::Vec3<float> position = {0.0f,0.0f,0.0f};
    /// Цвет (и яркость) источника света
    math::Vec3<float> color = {1.0f,1.0f,1.0f};
    /// Радиус источника (может быть использован для генерации мягких теней)
    float radius = 0.0f;
};

/**
 * \brief Элемент сцены
 */
class SceneElement
{
protected:
    /// Материал
    Material material_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    SceneElement():material_({
        {1.0f,1.0f,1.0f},
        0.0f,
        16.0f,
        1.0f,
        1.0f,
        1.0f
    }){}

    /**
     * \brief Основной конструктор
     * \param material Материал объекта
     */
    explicit SceneElement(const Material& material):material_(material){};

    /**
     * \brief Установить материал
     * \param material Материал объекта
     */
    void setMaterial(const Material& material)
    {
        this->material_ = material;
    }

    /**
     * \brief Получить материал
     * \return Ссылка на материал объекта
     */
    const Material& getMaterial()
    {
        return material_;
    }

    /**
     * \brief Деструктор
     */
    virtual ~SceneElement() = default;

    /**
     * \brief Пересечение объекта сцены и луча (полностью виртуальный метод)
     * \param ray Луч
     * \param tMin Минимальное расстояние
     * \param tMax Максимальное расстояние
     * \param tOut Итоговое расстояние до точки пересечения
     * \param normalOut Нормаль поверхности в точке пересечени
     * \return Было ли пересечение с объектом
     */
    virtual bool intersectsRay(const math::Ray& ray, float tMin, float tMax, float* tOut, math::Vec3<float>* normalOut) const = 0;
};