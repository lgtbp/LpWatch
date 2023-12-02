
#ifndef __LP_VERIFY_H
#define __LP_VERIFY_H

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief 系统跟证书初始化
    /// @param 无-内置了
    /// @return 0 ok
    int lptls_app_crt_chain_init(void);

    /// @brief 系统根证书添加次级证书
    /// @param ca_path 证书-buf
    /// @return 0 ok
    int lptls_app_crt_chain_add(char *tbuf_ca, int flen);

    /// @brief 系统根证书添加次级证书
    /// @param ca_path 证书路径
    /// @return 0 ok
    int lptls_app_crt_chain_add_form_file(char *ca_path);

    /// @brief 检测是不是系统根证书链上的
    /// @param ca_path 证书-
    /// @return 0 ok
    int lptls_app_crt_chain_check(char *tbuf_ca, int flen);

    /// @brief 验证这个app是不是合法的
    /// @param zip_file zip文件路径
    /// @return
    int lp_verify_zipfile(char *zip_file);

    /// @brief 验证这个app是合法的并升级
    /// @param zip_file zip文件路径
    /// @return
    int lp_verify_zipfile2up(char *zip_file);

    /// @brief 返回当前使用的证书信息
    /// @param tbuf 输出证书 上级颁发者 和 证书拥有者
    /// @param max_len tbuf长度
    /// @return 输出长度 0是没有数据
    int lptls_app_crt_info_verify(char *tbuf, int tbuf_len);

    /// @brief 是不是使用了测试证书
    /// @return 1 使用了测试证书
    int lptls_app_crt_lp_test_ca_if(void);

    /// @brief 系统升级 - 保证系统有升级文件 - 否则系统回退出厂程序
    /// @return >0 err
    int lp_sys_up(void);

    /// @brief 回退app
    int lp_lvgl_app_rollback(void);
    /// @brief 升级
    int lp_lvgl_app_up(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif