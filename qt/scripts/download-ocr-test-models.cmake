if(NOT DEFINED MODEL_DIRECTORY OR MODEL_DIRECTORY STREQUAL "")
    message(FATAL_ERROR "MODEL_DIRECTORY is required")
endif()
file(MAKE_DIRECTORY "${MODEL_DIRECTORY}")

# 1. 【OCR测试】【真实模型】沿用应用下载器的官方模型地址与 SHA-256
set(model_base "https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.8.0")
set(model_names ch_PP-OCRv5_det_mobile.onnx ch_PP-OCRv5_rec_mobile.onnx ppocrv5_dict.txt)
set(model_paths
    "onnx/PP-OCRv5/det/ch_PP-OCRv5_det_mobile.onnx"
    "onnx/PP-OCRv5/rec/ch_PP-OCRv5_rec_mobile.onnx"
    "paddle/PP-OCRv5/rec/ch_PP-OCRv5_rec_mobile/ppocrv5_dict.txt")
set(model_hashes
    "4d97c44a20d30a81aad087d6a396b08f786c4635742afc391f6621f5c6ae78ae"
    "5825fc7ebf84ae7a412be049820b4d86d77620f204a041697b0494669b1742c5"
    "d1979e9f794c464c0d2e0b70a7fe14dd978e9dc644c0e71f14158cdf8342af1b")
foreach(index RANGE 0 2)
    list(GET model_names ${index} name)
    list(GET model_paths ${index} path)
    list(GET model_hashes ${index} hash)
    file(DOWNLOAD "${model_base}/${path}" "${MODEL_DIRECTORY}/${name}"
        EXPECTED_HASH "SHA256=${hash}" TLS_VERIFY ON TIMEOUT 180)
endforeach()
