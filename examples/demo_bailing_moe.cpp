/**
 * @file demo_bailing_moe.cpp
 * @brief A demo for using Bailing MoE model.
 * @author Rongjie Yi
 * @date 2025-07-01
 *
 */
#include "cmdline.h"
#include "models/ling/configuration_bailing_moe.hpp"
#include "models/ling/modeling_bailing_moe.hpp"
#include "models/ling/tokenizer_bailing.hpp"

using namespace mllm;

int main(int argc, char **argv) {
    std::iostream::sync_with_stdio(false);

    cmdline::parser cmdParser;
    cmdParser.add<string>("vocab", 'v', "specify mllm tokenizer model path", false, "../vocab/ling_vocab.mllm");
    cmdParser.add<string>("merge", 'e', "specify mllm merge file path", false, "../vocab/ling_merges.txt");
#ifdef ARM
    cmdParser.add<string>("model", 'm', "specify mllm model path", false, "../models/ling-lite-1.5-kai_q4_0.mllm");
#else
    cmdParser.add<string>("model", 'm', "specify mllm model path", false, "../models/ling-lite-1.5-q4_0.mllm");
#endif
    cmdParser.add<int>("limits", 'l', "max KV cache size", false, 500);
    cmdParser.add<int>("thread", 't', "num of threads", false, 4);
    cmdParser.parse_check(argc, argv);

    string vocab_path = cmdParser.get<string>("vocab");
    string merge_path = cmdParser.get<string>("merge");
    string model_path = cmdParser.get<string>("model");
    int tokens_limit = cmdParser.get<int>("limits");
    CPUBackend::cpu_threads = cmdParser.get<int>("thread");

    auto tokenizer = BaiLingTokenizer(vocab_path, merge_path);
    BailingMoeConfig config(tokens_limit);
    // config.attn_implementation = "sage_attention";
    auto model = BailingMoeForCausalLM(config);
    model.load(model_path);

    vector<string> in_strs = {
        "怎样计算1+2+...+100的和？",
        "Who are you?",
        "Give me a short introduction to large language model.",
        "背诵天下第一骈文",
        "夕焼けとコオロギが一斉に飛び、秋水は共に天一色になる。上面句子翻译成中文",
        "你写一首七言绝句。",
        "背诵一下水调歌头。",
        "清晨的阳光透过薄纱窗帘，懒洋洋地洒在木地板上，空气中飘散着咖啡豆研磨后特有的醇厚香气。窗外传来几声清脆的鸟鸣，伴随着远处隐约的车流声，构成这座都市尚未完全苏醒的独特交响。书桌上摊开着昨夜未读完的书，书页边缘已微微卷起。厨房里，水壶正发出细密的声响，预示着一天的热饮即将就绪。昨日的计划表贴在冰箱门上，几个重要的待办事项用红笔醒目地圈出。公园里晨练的人们身影绰绰，有节奏的脚步声和太极音乐交织。一只橘猫敏捷地跃上围墙，在晨光中伸展着腰肢，神态悠闲得仿佛它是这片领地的主人。街角的面包店刚拉开铁门，新鲜出炉的面包香气迫不及待地涌向街头。公交站台上，等待的乘客低头刷着手机屏幕，神情各异。云朵缓慢地在湛蓝的天空中移动，时间似乎被拉长了片刻。生活就在这些微小的、平凡的细节里徐徐展开，既不惊天动地，却也充满细碎的温暖和实在的步履。新的一天开始了。\n​​请在以上文本中找出描述“气味”的句子（复制出来），然后判断叙述者对“橘猫”的态度是正面还是负面，最后请用三个成语概括文中描绘的早晨氛围。",
        "项羽已杀卿子冠军，威震楚国，名闻诸侯。乃遣当阳君、蒲将军将卒二万渡河，救巨鹿。战少利，陈馀复请兵。项羽乃悉引兵渡河，皆沉船，破釜甑，烧庐舍，持三日粮，以示士卒必死，无一还心。于是至则围王离，与秦军遇，九战，绝其甬道，大破之，杀苏角，虏王离。涉间不降楚，自烧杀。当是时，楚兵冠诸侯。诸侯军救巨鹿下者十余壁，莫敢纵兵。及楚击秦，诸将皆从壁上观。楚战士无不一以当十，楚兵呼声动天，诸侯军无不人人惴恐。于是已破秦军，项羽召见诸侯将，入辕门，无不膝行而前，莫敢仰视。项羽由是始为诸侯上将军，诸侯皆属焉。 问题：结合项羽在巨鹿之战中的战术决策与心理威慑手段，分析其如何实现『楚战士无不一以当十』的战斗效应，并论述这种军事心理学实践对诸侯将领『膝行而前，莫敢仰视』行为模式的生成机制。",
    };
    for (int i = 0; i < in_strs.size(); ++i) {
        auto input_str = tokenizer.apply_chat_template(in_strs[i]);
        auto input_tensor = tokenizer.tokenize(input_str);
        // std::cout << "[tks]" << input_tensor.sequence() << " tokens" << std::endl;
        std::cout << "[Q] " << in_strs[i] << std::endl;
        std::cout << "[A] " << std::flush;

        LlmTextGeneratorOpts opt{
            .max_new_tokens = static_cast<size_t>(tokens_limit - input_tensor.sequence()),
            .do_sample = false,
            .temperature = 0.3F,
            .top_k = 50,
            .top_p = 0.F,
        };
        model.generate(input_tensor, opt, [&](unsigned int out_token) -> bool {
            auto out_string = tokenizer.detokenize({out_token});
            auto [not_end, output_string] = tokenizer.postprocess(out_string);
            if (!not_end) { return false; }
            std::cout << output_string << std::flush;
            return true;
        });
        std::cout << "\n";
        model.clear_kvcache();
        model.profiling();
    }
}
