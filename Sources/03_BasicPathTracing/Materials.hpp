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
    explicit MaterialMetal(const math::Vec3<float>& albedo = {0.5f,0.5f,0.5f}, float roughness = 0.0f):
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

/**
 * \brief Диэлектрик (жидкость, прозрачные материалы)
 */
class MaterialDielectric : public Material
{
private:
    /// Коэффициент преломления
    float refractionIndex_;

    /**
     * \brief Подсчет степени отражения (Аппроксимация Шлика)
     * \param v Вектор падения луча
     * \param normal Нормаль поверхности
     * \param refractionRatio Коээфициент преломления вещества
     * \return Степень отражения
     */
    static float reflectance(const math::Vec3<float>& v, const math::Vec3<float>& normal, const float& refractionRatio)
    {
        // Использовать аппроксимациб Шлика для вычисления отражательной способности по углу падения и коэффициенту преломления
        float cosine = std::max(math::Dot(-v,normal),0.0f);
        auto r0 = (1.0f - refractionRatio) / (1.0f + refractionRatio);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * powf((1.0f - cosine),5);
    }

public:
    /**
     * \brief Конструктор по умолчанию
     */
    MaterialDielectric():Material(),refractionIndex_(1.0f){}

    /**
     * \brief Основной конструктор
     * \param refractionIndex Коэффициент преломления
     */
    explicit MaterialDielectric(const float& refractionIndex = 1.0f):Material(),refractionIndex_(refractionIndex){}

    /**
     * \brief Установить коэффициент преломления
     * \param refractionIndex Коэффициент преломления
     */
    void setRefractionIndex(const float& refractionIndex){
        this->refractionIndex_ = refractionIndex;
    }

    /**
     * \brief Получить коэффициент преломления
     * \return Коэффициент преломления
     */
    float getRefractionIndex() const{
        return refractionIndex_;
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
        // Коэффициент преломления инвертируется если луч выходит из вещества (удар о нелицевую сторону объекта)
        float refractionIndex = hitInfo.frontFaceSurface ? refractionIndex_ : 1.0f / refractionIndex_;

        // По умолчанию - луч отражается
        math::Vec3<float> scatteredDir = math::Reflect(rayIn.getDirection(), hitInfo.normal);

        // Если отражательной способности не достаточно - луч преломляется
        // Граница отражательной способности - случайное значение для каждого луча
        // Таким образом объект частично отражает, частично преломляет
        if(reflectance(rayIn.getDirection(),hitInfo.normal,1.0f/refractionIndex) < RndFloat()){
            scatteredDir = math::Refract(rayIn.getDirection(),hitInfo.normal,refractionIndex,true);
        }

        // Перенаправленный луч
        if(scatteredRayOut != nullptr){
            scatteredRayOut->setOrigin(hitInfo.point);
            scatteredRayOut->setDirection(scatteredDir);
        }

        // Нет затухания (свет преломленного луча передается как есть)
        if(attenuationOut){
            *attenuationOut = {1.0f,1.0f,1.0f};
        }

        // Луч перенаправлен
        return true;
    }
};