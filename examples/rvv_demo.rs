use density_rs::algorithms::chameleon::chameleon::Chameleon;
use density_rs::algorithms::cheetah::cheetah::Cheetah;
use density_rs::algorithms::lion::lion::Lion;

fn main() {
    println!("Density-rs RVV Optimization Demo");
    println!("================================");
    
    // Check RVV support status
    let rvv_supported = density_rs::is_rvv_available();
    println!("RVV Support Status: {}", if rvv_supported { "Supported" } else { "Not Supported" });
    
    // Test data
    let test_data = "This is a test string for demonstrating RVV optimization functionality.".repeat(100);
    println!("Test data size: {} bytes", test_data.len());
    
    // Prepare output buffers
    let mut compressed = vec![0u8; test_data.len() * 2]; // Allocate enough space
    let mut decompressed = vec![0u8; test_data.len()];
    
    println!("\n=== Chameleon Algorithm Test ===");
    test_algorithm("Chameleon", &test_data, &mut compressed, &mut decompressed, 
        |input, output| Chameleon::encode(input, output),
        |input, output| Chameleon::decode(input, output));
    
    println!("\n=== Cheetah Algorithm Test ===");
    test_algorithm("Cheetah", &test_data, &mut compressed, &mut decompressed,
        |input, output| Cheetah::encode(input, output),
        |input, output| Cheetah::decode(input, output));
    
    println!("\n=== Lion Algorithm Test ===");
    test_algorithm("Lion", &test_data, &mut compressed, &mut decompressed,
        |input, output| Lion::encode(input, output),
        |input, output| Lion::decode(input, output));
    
    if rvv_supported {
        println!("\n✅ RVV optimization is enabled, performance has been improved!");
    } else {
        println!("\n⚠️  RVV optimization is not enabled, using standard implementation.");
        println!("Tip: Use --features rvv on RISC-V platform to enable optimization.");
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
    // Encoding
    let start = std::time::Instant::now();
    let compressed_size = encode_fn(test_data.as_bytes(), compressed)
        .expect("Encoding failed");
    let encode_time = start.elapsed();
    
    // Decoding
    let start = std::time::Instant::now();
    let decompressed_size = decode_fn(&compressed[..compressed_size], decompressed)
        .expect("Decoding failed");
    let decode_time = start.elapsed();
    
    // Verification
    let original_data = test_data.as_bytes();
    let recovered_data = &decompressed[..decompressed_size];
    assert_eq!(original_data, recovered_data, "Data verification failed");
    
    // Statistics
    let compression_ratio = test_data.len() as f64 / compressed_size as f64;
    
    println!("{} Results:", name);
    println!("  Original size:  {} bytes", test_data.len());
    println!("  Compressed size: {} bytes", compressed_size);
    println!("  Compression ratio: {:.2}x", compression_ratio);
    println!("  Encoding time:  {:?}", encode_time);
    println!("  Decoding time:  {:?}", decode_time);
    println!("  Verification:   ✅ Passed");
}