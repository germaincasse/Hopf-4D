#include "scene/SceneSerializer.h"

#include "core/Logger.h"
#include "geometry/Primitives.h"
#include "scene/Scene.h"
#include "scripting/ScriptRegistry.h"

#include <fstream>
#include <sstream>
#include <string>

namespace hopf::scene {

namespace {

// Reads up to the end of line after `in` has just consumed a leading keyword.
// Returns the rest with leading whitespace stripped. Used for string-valued
// fields like names that may contain spaces.
std::string readRestOfLine(std::istringstream& in) {
    std::string s;
    std::getline(in, s);
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return s.substr(i);
}

void writeVec4(std::ostream& o, const math::Vec4& v) {
    o << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.w;
}

math::Vec4 readVec4(std::istringstream& in) {
    math::Vec4 v;
    in >> v.x >> v.y >> v.z >> v.w;
    return v;
}

} // namespace

bool saveScene(const Scene& scene, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        core::Logger::error("saveScene: cannot open %s", path.c_str());
        return false;
    }
    out << "hopf-scene 1\n";
    out << "sceneName " << scene.name() << '\n';
    for (const auto& e : scene.entities()) {
        out << "entity\n";
        out << "  id " << e.id << '\n';
        out << "  parent " << e.parent << '\n';
        out << "  name " << e.name << '\n';
        out << "  pos ";   writeVec4(out, e.transform.position); out << '\n';
        const auto& r = e.transform.rotation;
        out << "  rot " << r.xy << ' ' << r.xz << ' ' << r.xw << ' '
                        << r.yz << ' ' << r.yw << ' ' << r.zw << '\n';
        out << "  scale "; writeVec4(out, e.transform.scale); out << '\n';
        out << "  visible " << (e.visible ? 1 : 0) << '\n';
        const auto& ar = e.autoRotate;
        out << "  autoRotate " << ar.xy << ' ' << ar.xz << ' ' << ar.xw << ' '
                                << ar.yz << ' ' << ar.yw << ' ' << ar.zw << '\n';
        if (!e.primitiveType.empty()) out << "  primitive " << e.primitiveType << '\n';

        if (e.light) {
            const auto& l = *e.light;
            out << "  light " << l.intensity << ' ' << l.r << ' ' << l.g << ' ' << l.b << '\n';
        }
        if (e.camera2D) {
            const auto& c = *e.camera2D;
            out << "  camera2D " << c.orthoSize << ' ' << c.zNear << ' ' << c.zFar
                << ' ' << static_cast<int>(c.displayStyle)
                << ' ' << c.bg.r << ' ' << c.bg.g << ' ' << c.bg.b << '\n';
        }
        if (e.camera3D) {
            const auto& c = *e.camera3D;
            out << "  camera3D " << (c.perspective ? 1 : 0) << ' ' << c.fovYDeg << ' '
                << c.orthoHalfH << ' ' << c.zNear << ' ' << c.zFar
                << ' ' << static_cast<int>(c.displayStyle)
                << ' ' << c.bg.r << ' ' << c.bg.g << ' ' << c.bg.b << '\n';
        }
        if (e.camera4D) {
            const auto& c = *e.camera4D;
            out << "  camera4D " << static_cast<int>(c.mode)
                << ' ' << static_cast<int>(c.projection)
                << ' ' << c.focal4 << ' ' << c.wOffset
                << ' ' << c.sliceAxis.x << ' ' << c.sliceAxis.y
                << ' ' << c.sliceAxis.z << ' ' << c.sliceAxis.w
                << ' ' << c.sliceVal
                << ' ' << (c.isMain ? 1 : 0)
                << ' ' << static_cast<int>(c.displayStyle)
                << ' ' << c.bg.r << ' ' << c.bg.g << ' ' << c.bg.b << '\n';
        }
        if (e.collider) {
            out << "  collider " << e.collider->dimension << ' '
                << (e.collider->isTrigger ? 1 : 0) << '\n';
        }
        if (e.tint) {
            out << "  tint " << e.tint->r << ' ' << e.tint->g << ' ' << e.tint->b << '\n';
        }
        if (e.rigidbody) {
            const auto& rb = *e.rigidbody;
            out << "  rigidbody " << rb.dimension << ' ' << rb.mass << ' '
                << (rb.kinematic ? 1 : 0) << ' ' << (rb.useGravity ? 1 : 0) << '\n';
        }
        if (e.audioSource) {
            const auto& a = *e.audioSource;
            // Bus first (single word), then clipPath last (rest of line).
            out << "  audioSource " << a.volume << ' ' << a.pitch << ' '
                << (a.loop ? 1 : 0) << ' ' << (a.playOnStart ? 1 : 0) << ' '
                << (a.spatial ? 1 : 0) << ' ' << a.maxDistance << ' '
                << (a.bus.empty() ? "master" : a.bus) << ' ' << a.clipPath << '\n';
        }
        if (e.audioListener) {
            out << "  audioListener " << (e.audioListener->active ? 1 : 0) << '\n';
        }
        if (e.uiRect) {
            const auto& u = *e.uiRect;
            out << "  uiRect " << static_cast<int>(u.anchor) << ' '
                << u.x << ' ' << u.y << ' ' << u.width << ' ' << u.height << ' '
                << u.r << ' ' << u.g << ' ' << u.b << ' ' << u.a
                << ' ' << (u.worldSpace ? 1 : 0) << '\n';
        }
        if (e.uiText) {
            const auto& u = *e.uiText;
            out << "  uiText " << u.fontSize << ' '
                << u.r << ' ' << u.g << ' ' << u.b << ' ' << u.a << ' ' << u.text << '\n';
        }
        if (e.uiImage) {
            const auto& u = *e.uiImage;
            out << "  uiImage " << u.r << ' ' << u.g << ' ' << u.b << ' ' << u.a
                << ' ' << u.imagePath << '\n';
        }
        if (e.uiButton) {
            const auto& u = *e.uiButton;
            const std::string scriptName = u.onClickScript.empty() ? "-" : u.onClickScript;
            const std::string methodName = u.onClickMethod.empty() ? "-" : u.onClickMethod;
            out << "  uiButton " << u.r << ' ' << u.g << ' ' << u.b << ' ' << u.a << ' '
                << u.hoverR << ' ' << u.hoverG << ' ' << u.hoverB << ' ' << u.hoverA << ' '
                << scriptName << ' ' << methodName << '\n';
        }
        for (const auto& s : e.scripts) {
            out << "  script " << s.typeName << '\n';
        }
        out << "end\n";
    }
    return true;
}

bool loadScene(Scene& scene, const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        core::Logger::error("loadScene: cannot open %s", path.c_str());
        return false;
    }
    std::string header;
    int version = 0;
    in >> header >> version;
    if (header != "hopf-scene" || version != 1) {
        core::Logger::error("loadScene: not a hopf-scene v1 file: %s", path.c_str());
        return false;
    }
    in.ignore(); // skip trailing newline

    scene.clear();

    auto& reg = scripting::ScriptRegistry::instance();

    std::string raw;
    Entity pending;
    bool inEntity = false;

    auto commitEntity = [&] {
        if (!inEntity) return;
        scene.entities().push_back(std::move(pending));
        pending = Entity{};
        inEntity = false;
    };

    std::string sceneName = "Loaded";
    while (std::getline(in, raw)) {
        size_t s = 0;
        while (s < raw.size() && (raw[s] == ' ' || raw[s] == '\t')) ++s;
        if (s == raw.size() || raw[s] == '#') continue;
        std::istringstream ls(raw.substr(s));
        std::string tok;
        ls >> tok;

        if (tok == "sceneName") {
            sceneName = readRestOfLine(ls);
        } else if (tok == "entity") {
            commitEntity();
            inEntity = true;
            pending = Entity{};
        } else if (tok == "end") {
            commitEntity();
        } else if (!inEntity) {
            // Ignore unknown top-level tokens.
            continue;
        } else if (tok == "id")        { ls >> pending.id; }
        else if (tok == "parent")      { ls >> pending.parent; }
        else if (tok == "name")        { pending.name = readRestOfLine(ls); }
        else if (tok == "pos")         { pending.transform.position = readVec4(ls); }
        else if (tok == "rot") {
            auto& r = pending.transform.rotation;
            ls >> r.xy >> r.xz >> r.xw >> r.yz >> r.yw >> r.zw;
        }
        else if (tok == "scale")       { pending.transform.scale = readVec4(ls); }
        else if (tok == "visible")     { int v; ls >> v; pending.visible = (v != 0); }
        else if (tok == "autoRotate") {
            auto& a = pending.autoRotate;
            ls >> a.xy >> a.xz >> a.xw >> a.yz >> a.yw >> a.zw;
        }
        else if (tok == "primitive") {
            std::string name; ls >> name;
            geometry::PrimitiveType pt;
            if (geometry::parsePrimitiveType(name.c_str(), pt)) {
                pending.mesh = scene.takeOwnedMesh(geometry::buildPrimitive(pt));
                pending.primitiveType = name;
            } else {
                core::Logger::error("loadScene: unknown primitive '%s'", name.c_str());
            }
        }
        else if (tok == "light") {
            DirectionalLight l;
            ls >> l.intensity >> l.r >> l.g >> l.b;
            pending.light = l;
        }
        else if (tok == "camera2D") {
            Camera2DComponent c;
            int ds = static_cast<int>(c.displayStyle);
            ls >> c.orthoSize >> c.zNear >> c.zFar >> ds >> c.bg.r >> c.bg.g >> c.bg.b;
            c.displayStyle = static_cast<CameraDisplayStyle>(ds);
            pending.camera2D = c;
        }
        else if (tok == "camera3D") {
            Camera3DComponent c;
            int pers; ls >> pers; c.perspective = (pers != 0);
            int ds = static_cast<int>(c.displayStyle);
            ls >> c.fovYDeg >> c.orthoHalfH >> c.zNear >> c.zFar >> ds
               >> c.bg.r >> c.bg.g >> c.bg.b;
            c.displayStyle = static_cast<CameraDisplayStyle>(ds);
            pending.camera3D = c;
        }
        else if (tok == "camera4D") {
            Camera4DComponent c;
            int modeInt = 0;
            int projInt = static_cast<int>(c.projection);
            int isMain  = 0;
            int ds      = static_cast<int>(c.displayStyle);
            ls >> modeInt >> projInt >> c.focal4 >> c.wOffset
               >> c.sliceAxis.x >> c.sliceAxis.y >> c.sliceAxis.z >> c.sliceAxis.w
               >> c.sliceVal >> isMain >> ds >> c.bg.r >> c.bg.g >> c.bg.b;
            c.mode         = static_cast<Camera4DComponent::Mode>(modeInt);
            c.projection   = static_cast<CameraProjection>(projInt);
            c.isMain       = (isMain != 0);
            c.displayStyle = static_cast<CameraDisplayStyle>(ds);
            pending.camera4D = c;
        }
        else if (tok == "collider") {
            MeshColliderComponent mc;
            int trig; ls >> mc.dimension >> trig; mc.isTrigger = (trig != 0);
            pending.collider = mc;
        }
        else if (tok == "tint") {
            ColorTint t;
            ls >> t.r >> t.g >> t.b;
            pending.tint = t;
        }
        else if (tok == "rigidbody") {
            RigidbodyComponent rb;
            int kine, gravity;
            ls >> rb.dimension >> rb.mass >> kine >> gravity;
            rb.kinematic  = (kine != 0);
            rb.useGravity = (gravity != 0);
            pending.rigidbody = rb;
        }
        else if (tok == "audioSource") {
            AudioSourceComponent a;
            int loop, pos, spatial;
            ls >> a.volume >> a.pitch >> loop >> pos >> spatial >> a.maxDistance >> a.bus;
            a.loop = (loop != 0); a.playOnStart = (pos != 0); a.spatial = (spatial != 0);
            a.clipPath = readRestOfLine(ls);
            pending.audioSource = a;
        }
        else if (tok == "audioListener") {
            AudioListenerComponent al;
            int act; ls >> act; al.active = (act != 0);
            pending.audioListener = al;
        }
        else if (tok == "uiRect") {
            UIRectComponent u;
            int anchor;
            int worldSpace = 0;
            ls >> anchor >> u.x >> u.y >> u.width >> u.height
               >> u.r >> u.g >> u.b >> u.a >> worldSpace;
            u.anchor     = static_cast<UIAnchor>(anchor);
            u.worldSpace = (worldSpace != 0);
            pending.uiRect = u;
        }
        else if (tok == "uiText") {
            UITextComponent u;
            ls >> u.fontSize >> u.r >> u.g >> u.b >> u.a;
            u.text = readRestOfLine(ls);
            pending.uiText = u;
        }
        else if (tok == "uiImage") {
            UIImageComponent u;
            ls >> u.r >> u.g >> u.b >> u.a;
            u.imagePath = readRestOfLine(ls);
            pending.uiImage = u;
        }
        else if (tok == "uiButton") {
            UIButtonComponent u;
            std::string sn, mn;
            ls >> u.r >> u.g >> u.b >> u.a
               >> u.hoverR >> u.hoverG >> u.hoverB >> u.hoverA
               >> sn >> mn;
            u.onClickScript = (sn == "-") ? "" : sn;
            u.onClickMethod = (mn == "-") ? "" : mn;
            pending.uiButton = u;
        }
        else if (tok == "script") {
            std::string sn; ls >> sn;
            ScriptInstance si;
            si.typeName = sn;
            si.instance = reg.create(sn);
            if (si.instance) pending.scripts.push_back(std::move(si));
        }
        else {
            // Unknown key inside an entity block: skip silently to stay forward-compatible.
        }
    }
    commitEntity();

    scene.setName(sceneName);
    scene.refreshNextIdFromContents();
    return true;
}

} // namespace hopf::scene
