#pragma once

#include "Utils.h"

/**
 * \brief Рассеивающий материал
 */
class MaterialDiffuse : public Material
{
private:
    /// Собственный цвет материала (для затухания он должен быть ниже 1)
    math::Vec3<float> albedo_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    MaterialDiffuse():Material(),albedo_({0.0f,0.0f,0.0f}){};

    /**
     * \brief Конструктор по умолчанию
     * \param albedo  Собственный цвет материала
     */
    explicit MaterialDiffuse(const math::Vec3<float>& albedo = {0.5f,0.5f,0.5f}):Material(),albedo_(albedo){}

    /**
     * \brief Установка собственного цвета (albedo)
     * \param albedo Цвет
     */
    void setAlbedo(const math::Vec3<float>& albedo){
        this->albedo_ = albedo;
    }

    /**
     * \brief Получение собственного цвета (albedo)
     * \return Цвет
     */
    const math::Vec3<float>& getAlbedo() const{
        return albedo_;
    }

    /**
     * \brief Разброс луча
     * \param rayIn Входной луч
     * \param hitInfo Информация о пересечении
     * \param attenuationOut Затухание
     * \param scatteredRayOut Итоговый луч
     * \return Переотражается ли луч или поглощается
     */
    bool scatter(
            const math::Ray& rayIn,
            const HitInfo& hitInfo,
            math::Vec3<float>* attenuationOut,
            math::Ray* scatteredRayOut) const override
    {
        // Входной луч не используется
        (void) rayIn;

        // Вектор направления переотраженного луча (случайное направление в пределах полусферы)
        math::Vec3<float> scatteredDir = RndHemisphereVec(hitInfo.normal);

        // Переотраженный поверхностью луч
        if(scatteredRayOut != nullptr){
            scatteredRayOut->setOrigin(hitInfo.point);
            scatteredRayOut->setDirection(scatteredDir);
        }

        // Затухание луча (для затухания цвет albedo должен быть ниже 1)
        if(attenuationOut != nullptr){
            *attenuationOut = albedo_;
        }

        // Луч переотражается
        return true;
    }
};

/**
 * \brief Металлический материал
 */
class MaterialMetal : public Material
{
private:
    /// Собственный цвет материала (для затухания он должен быть ниже 1)
    math::Vec3<float> albedo_;
    /// Шероховатость повехрности
    float roughness_;

public:
    /**
     * \brief Конструктор по умолчанию
     */
    MaterialMetal():Material(),albedo_({0.0f,0.0f,0.0f}),roughness_(0.0f){};

    /**
     * \brief Конструктор по умолчанию
     * \param albedo  Собственный цвет материала
     */
    explicit MaterialMetal(const math::Vec3<float>& albedo = {0.5f,0.5f,0.5f},float roughness = 0.0f):
    Material(),albedo_(albedo),roughness_(roughness){}

    /**
     * \brief Установка собственного цвета (albedo)
     * \param albedo Цвет
     */
    void setAlbedo(const math::Vec3<float>& albedo){
        this->albedo_ = albedo;
    }

    /**
     * \brief Получение собственного цвета (albedo)
     * \return Цвет
     */
    const math::Vec3<float>& getAlbedo() const{
        return albedo_;
    }

    /**
     * \brief Установить шероховатость
     * \param roughness Шероховатость
     */
    void setRoughness(float roughness){
        this->roughness_ = roughness;
    }

    /**
     * \brief Получить шероховатость поверхности
     * \return Шероховатость
     */
    float getRoughness() const{
        return roughness_;
    }

    /**
     * \brief Разброс луча
     * \param rayIn Входной луч
     * \param hitInfo Информация о пересечении
     * \param attenuationOut Затухание
     * \param scatteredRayOut Итоговый луч
     * \return Переотражается ли луч или поглощается
     */
    bool scatter(
            const math::Ray& rayIn,
            const HitInfo& hitInfo,
            math::Vec3<float>* attenuationOut,
            math::Ray* scatteredRayOut) const override
    {
        // Вектор направления переотраженного луча (отражение луча ровно по нормали)
        math::Vec3<float> scatteredDir = math::Reflect(rayIn.getDirection(),hitInfo.normal);

        // Если шероховатость больше чем 0 - появляется разброс луча вдоль отраженного направления
        if(roughness_ > 0.0f){
            scatteredDir = RndHemisphereVec(scatteredDir,60.0f * roughness_);
        }

        // Переотраженный поверхностью луч
        if(scatteredRayOut != nullptr){
            scatteredRayOut->setOrigin(hitInfo.point);
            scatteredRayOut->setDirection(scatteredDir);
        }

        // Затухание луча (для затухания цвет albedo должен быть ниже 1)
        if(attenuationOut != nullptr){
            *attenuationOut = albedo_;
        }

        // Луч переотражается
        return true;
    }
};