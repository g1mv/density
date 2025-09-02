use density_rs::algorithms::chameleon::chameleon::Chameleon;
use density_rs::algorithms::cheetah::cheetah::Cheetah;
use density_rs::algorithms::lion::lion::Lion;

fn main() {
    println!("Density-rs RVV 优化演示");
    println!("========================");
    
    // 检查 RVV 支持状态
    let rvv_supported = density_rs::is_rvv_available();
    println!("RVV 支持状态: {}", if rvv_supported { "支持" } else { "不支持" });
    
    // 测试数据
    let test_data = "这是一个测试字符串，用于演示 RVV 优化功能。".repeat(100);
    println!("测试数据大小: {} 字节", test_data.len());
    
    // 准备输出缓冲区
    let mut compressed = vec![0u8; test_data.len() * 2]; // 给足够的空间
    let mut decompressed = vec![0u8; test_data.len()];
    
    println!("\n=== Chameleon 算法测试 ===");
    test_algorithm("Chameleon", &test_data, &mut compressed, &mut decompressed, 
        |input, output| Chameleon::encode(input, output),
        |input, output| Chameleon::decode(input, output));
    
    println!("\n=== Cheetah 算法测试 ===");
    test_algorithm("Cheetah", &test_data, &mut compressed, &mut decompressed,
        |input, output| Cheetah::encode(input, output),
        |input, output| Cheetah::decode(input, output));
    
    println!("\n=== Lion 算法测试 ===");
    test_algorithm("Lion", &test_data, &mut compressed, &mut decompressed,
        |input, output| Lion::encode(input, output),
        |input, output| Lion::decode(input, output));
    
    if rvv_supported {
        println!("\n✅ RVV 优化已启用，性能得到了提升！");
    } else {
        println!("\n⚠️  RVV 优化未启用，使用标准实现。");
        println!("提示：在 RISC-V 平台上使用 --features rvv 来启用优化。");
    }
}

fn test_algorithm<E, D>(
    name: &str,
    test_data: &str,
    compressed: &mut [u8],
    decompressed: &mut [u8],
    encode_fn: E,
    decode_fn: D,
) 
where
    E: Fn(&[u8], &mut [u8]) -> Result<usize, density_rs::errors::encode_error::EncodeError>,
    D: Fn(&[u8], &mut [u8]) -> Result<usize, density_rs::errors::decode_error::DecodeError>,
{
    // 编码
    let start = std::time::Instant::now();
    let compressed_size = encode_fn(test_data.as_bytes(), compressed)
        .expect("编码失败");
    let encode_time = start.elapsed();
    
    // 解码
    let start = std::time::Instant::now();
    let decompressed_size = decode_fn(&compressed[..compressed_size], decompressed)
        .expect("解码失败");
    let decode_time = start.elapsed();
    
    // 验证
    let original_data = test_data.as_bytes();
    let recovered_data = &decompressed[..decompressed_size];
    assert_eq!(original_data, recovered_data, "数据验证失败");
    
    // 统计
    let compression_ratio = test_data.len() as f64 / compressed_size as f64;
    
    println!("{} 结果:", name);
    println!("  原始大小:   {} 字节", test_data.len());
    println!("  压缩大小:   {} 字节", compressed_size);
    println!("  压缩比:     {:.2}x", compression_ratio);
    println!("  编码时间:   {:?}", encode_time);
    println!("  解码时间:   {:?}", decode_time);
    println!("  验证:       ✅ 通过");
}