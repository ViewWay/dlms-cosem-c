# DLMS/COSEM C99 Embedded Library

纯 C99 实现的 DLMS/COSEM 协议栈，面向嵌入式系统设计。

## 特性

- **纯 C99** - 无动态内存分配，零外部依赖
- **嵌入式友好** - 栈分配，固定大小缓冲区，可配置宏
- **SM4 国密** - 完整的 SM4 查表实现（S-box + CTR/CBC/GCM）
- **AES-128** - 查表实现，支持 GCM/GMAC
- **模块化** - 可裁剪，不需要的模块可以不编译
- **240+ 测试** - 覆盖所有核心功能

## 模块

| 模块 | 说明 |
|------|------|
| `dlms_core` | OBIS 码、数据类型、缓冲区、工具函数 |
| `dlms_hdlc` | HDLC 帧编解码、CRC-16、字节填充、流式解析 |
| `dlms_axdr` | A-XDR 编解码，支持所有 DLMS 数据类型 |
| `dlms_asn1` | BER-TLV、AARQ/AARE 关联请求/响应 |
| `dlms_security` | SM4/AES-128、GCM/GMAC、HLS-ISM、安全控制 |
| `dlms_cosem` | COSEM 接口类（Data/Register/Clock/SecuritySetup/ProfileGeneric） |
| `dlms_client` | DLMS 客户端（关联/读/写/动作） |

## 快速开始

```bash
# 编译
make all

# 运行测试
make test

# 运行示例
make example

# CMake
mkdir build && cd build
cmake ..
make
ctest
```

## API 示例

### OBIS 码

```c
dlms_obis_t obis;
dlms_obis_from_string("0.0.1.8.0.255", &obis);
char buf[32];
dlms_obis_to_string(&obis, buf, sizeof(buf));
```

### HDLC 帧编解码

```c
dlms_hdlc_address_t server = { .logical_address = 1 };
dlms_hdlc_address_t client = { .logical_address = 16 };
uint8_t frame[128];
int len = dlms_hdlc_build_snrm(frame, sizeof(frame), &server, &client, &params);
```

### AXDR 编解码

```c
dlms_value_t val;
dlms_value_set_int32(&val, 42);
uint8_t buf[16];
size_t w;
dlms_axdr_encode(&val, buf, sizeof(buf), &w);
```

### SM4 加密

```c
uint8_t key[16], iv[12], plain[16], cipher[16], tag[12];
dlms_sm4_gcm_encrypt(key, iv, NULL, 0, plain, 16, cipher, tag);
```

### COSEM 对象

```c
dlms_data_object_t data;
dlms_obis_t ln = {{0,0,1,0,0,255}};
dlms_data_create(&data, &ln);
dlms_value_set_int32(&data_val, 100);
dlms_data_set_value(&data, &data_val);
```

## 编译配置

通过宏自定义缓冲区大小：

```c
#define DLMS_HDLC_MAX_FRAME_SIZE  512
#define DLMS_HDLC_MAX_INFO_LEN    400
#define DLMS_COSEM_MAX_OBJECTS    128
```

## 安全套件支持

| Suite | 算法 | 状态 |
|-------|------|------|
| 0 | AES-128-GCM | ✅ |
| 1 | AES-128-GCM | ✅ |
| 2 | AES-256-GCM | ⚠️ 密钥派生 |
| 5 | SM4-GCM | ✅ |

## 许可证

MIT License

## 参考

- IEC 62056-53 (DLMS/COSEM Application Layer)
- IEC 62056-46 (HDLC)
- GB/T 32907-2016 (SM4)
- DLMS UA 1000-1 (Blue Book)
