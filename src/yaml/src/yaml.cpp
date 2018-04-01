#include <primitives/yaml.h>

// no links allowed
// to do this we call YAML::Clone()
void merge(yaml &dst, const yaml &src, const YamlMergeFlags &flags)
{
    if (!src.IsDefined())
        return;

    // if 'dst' node is not a map, make it so
    if (!dst.IsMap())
        dst = yaml();

    for (const auto &f : src)
    {
        auto sf = f.first.as<String>();
        auto ff = f.second.Type();

        bool found = false;
        for (auto t : dst)
        {
            const auto st = t.first.as<String>();
            if (sf != st)
                continue;

            const auto ft = t.second.Type();
            if (ff == YAML::NodeType::Scalar && ft == YAML::NodeType::Scalar)
            {
                switch (flags.scalar_scalar)
                {
                case YamlMergeFlags::ScalarsToSet:
                {
                    yaml nn;
                    nn.push_back(t.second);
                    nn.push_back(f.second);
                    dst[st] = nn;
                    break;
                }
                case YamlMergeFlags::OverwriteScalars:
                    dst[st] = YAML::Clone(src[sf]);
                    break;
                case YamlMergeFlags::DontTouchScalars:
                    break;
                }
            }
            else if (ff == YAML::NodeType::Scalar && ft == YAML::NodeType::Sequence)
            {
                t.second.push_back(f.second);
            }
            else if (ff == YAML::NodeType::Sequence && ft == YAML::NodeType::Scalar)
            {
                yaml nn = YAML::Clone(f);
                nn.push_back(t.second);
                dst[st] = nn;
            }
            else if (ff == YAML::NodeType::Sequence && ft == YAML::NodeType::Sequence)
            {
                for (const auto &fv : f.second)
                    t.second.push_back(YAML::Clone(fv));
            }
            else if (ff == YAML::NodeType::Map && ft == YAML::NodeType::Map)
                merge(t.second, f.second, flags);
            else // elaborate more on this?
                throw std::runtime_error("yaml merge: nodes ('" + sf + "') has incompatible types");
            found = true;
        }
        if (!found)
        {
            dst[sf] = YAML::Clone(f.second);
        }
    }
}
