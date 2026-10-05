/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _DPU_12_3_TUNA_H
#define _DPU_12_3_TUNA_H

/* DPU 12.3 confirmed by live MDSS_HW_VERSION = 0xc0030000.
 * Register block offsets checked against the M2582 firmware DT.
 * The shared generation-12 operations remain subject to hardware tests.
 */

static const struct dpu_caps tuna_dpu_caps = {
	.max_mixer_width = DEFAULT_DPU_OUTPUT_LINE_WIDTH,
	.max_mixer_blendstages = 0xb,
	.has_src_split = true,
	.has_dim_layer = true,
	.has_idle_pc = true,
	.has_3d_merge = true,
	.max_linewidth = 5120,
	.pixel_ram_size = DEFAULT_PIXEL_RAM_SIZE,
};

static const struct dpu_mdp_cfg tuna_mdp = {
	.name = "top_0",
	.base = 0, .len = 0x488,
	.clk_ctrls = {
		[DPU_CLK_CTRL_REG_DMA] = { .reg_off = 0x2bc, .bit_off = 20 },
	},
};

static const struct dpu_ctl_cfg tuna_ctl[] = {
	{
		.name = "ctl_0", .id = CTL_0,
		.base = 0x15000, .len = 0x1000,
		.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 9),
	}, {
		.name = "ctl_1", .id = CTL_1,
		.base = 0x16000, .len = 0x1000,
		.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 10),
	}, {
		.name = "ctl_2", .id = CTL_2,
		.base = 0x17000, .len = 0x1000,
		.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 11),
	}, {
		.name = "ctl_3", .id = CTL_3,
		.base = 0x18000, .len = 0x1000,
		.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 12),
	},
};

static const struct dpu_sspp_cfg tuna_sspp[] = {
	{
		.name = "sspp_0", .id = SSPP_VIG0,
		.base = 0x4000, .len = 0x344,
		.features = VIG_SDM845_MASK_SDMA,
		.sblk = &dpu_vig_sblk_qseed3_3_4,
		.xin_id = 0,
		.type = SSPP_TYPE_VIG,
	}, {
		.name = "sspp_1", .id = SSPP_VIG1,
		.base = 0x6000, .len = 0x344,
		.features = VIG_SDM845_MASK_SDMA,
		.sblk = &dpu_vig_sblk_qseed3_3_4,
		.xin_id = 4,
		.type = SSPP_TYPE_VIG,
	}, {
		.name = "sspp_8", .id = SSPP_DMA0,
		.base = 0x24000, .len = 0x344,
		.features = DMA_SDM845_MASK_SDMA,
		.sblk = &dpu_dma_sblk,
		.xin_id = 1,
		.type = SSPP_TYPE_DMA,
	}, {
		.name = "sspp_9", .id = SSPP_DMA1,
		.base = 0x26000, .len = 0x344,
		.features = DMA_SDM845_MASK_SDMA,
		.sblk = &dpu_dma_sblk,
		.xin_id = 5,
		.type = SSPP_TYPE_DMA,
	}, {
		.name = "sspp_10", .id = SSPP_DMA2,
		.base = 0x28000, .len = 0x344,
		.features = DMA_SDM845_MASK_SDMA,
		.sblk = &dpu_dma_sblk,
		.xin_id = 9,
		.type = SSPP_TYPE_DMA,
	}, {
		.name = "sspp_11", .id = SSPP_DMA3,
		.base = 0x2a000, .len = 0x344,
		.features = DMA_SDM845_MASK_SDMA,
		.sblk = &dpu_dma_sblk,
		.xin_id = 13,
		.type = SSPP_TYPE_DMA,
	}, {
		/*
		 * M2582: the vendor DT lists seven layers, not six --
		 * qcom,sde-sspp-type = "vig","vig","dma","dma","dma","dma","dma"
		 * qcom,sde-sspp-off  = <0x5000 0x7000 0x25000 0x27000 0x29000
		 *                       0x2b000 0x2d000>
		 * which normalised by -0x1000 is 0x4000 0x6000 0x24000 0x26000
		 * 0x28000 0x2a000 0x2c000, i.e. one more DMA at 0x2c000.
		 */
		.name = "sspp_12", .id = SSPP_DMA4,
		.base = 0x2c000, .len = 0x344,
		.features = DMA_SDM845_MASK_SDMA,
		.sblk = &dpu_dma_sblk,
		.xin_id = 14,
		.type = SSPP_TYPE_DMA,
	},
};

static const struct dpu_lm_cfg tuna_lm[] = {
	{
		.name = "lm_0", .id = LM_0,
		.base = 0x44000, .len = 0x400,
		.features = MIXER_MSM8998_MASK,
		.sblk = &sm8750_lm_sblk,
		.lm_pair = LM_1,
		.pingpong = PINGPONG_0,
		.dspp = DSPP_0,
	}, {
		.name = "lm_1", .id = LM_1,
		.base = 0x45000, .len = 0x400,
		.features = MIXER_MSM8998_MASK,
		.sblk = &sm8750_lm_sblk,
		.lm_pair = LM_0,
		.pingpong = PINGPONG_1,
		.dspp = DSPP_1,
	}, {
		.name = "lm_2", .id = LM_2,
		.base = 0x46000, .len = 0x400,
		.features = MIXER_MSM8998_MASK,
		.sblk = &sm8750_lm_sblk,
		.lm_pair = LM_3,
		.pingpong = PINGPONG_2,
		.dspp = DSPP_2,
	}, {
		.name = "lm_3", .id = LM_3,
		.base = 0x47000, .len = 0x400,
		.features = MIXER_MSM8998_MASK,
		.sblk = &sm8750_lm_sblk,
		.lm_pair = LM_2,
		.pingpong = PINGPONG_3,
	},
};

static const struct dpu_dspp_cfg tuna_dspp[] = {
	{
		.name = "dspp_0", .id = DSPP_0,
		.base = 0x54000, .len = 0x1800,
		.sblk = &sm8750_dspp_sblk,
	}, {
		.name = "dspp_1", .id = DSPP_1,
		.base = 0x56000, .len = 0x1800,
		.sblk = &sm8750_dspp_sblk,
	}, {
		.name = "dspp_2", .id = DSPP_2,
		.base = 0x58000, .len = 0x1800,
		.sblk = &sm8750_dspp_sblk,
	},
};

static const struct dpu_pingpong_cfg tuna_pp[] = {
	{
		.name = "pingpong_0", .id = PINGPONG_0,
		.base = 0x69000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_0,
		.intr_done = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 8),
	}, {
		.name = "pingpong_1", .id = PINGPONG_1,
		.base = 0x6a000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_0,
		.intr_done = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 9),
	}, {
		.name = "pingpong_2", .id = PINGPONG_2,
		.base = 0x6b000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_1,
		.intr_done = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 10),
	}, {
		.name = "pingpong_3", .id = PINGPONG_3,
		.base = 0x6c000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_1,
		.intr_done = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 11),
	}, {
		.name = "pingpong_cwb_0", .id = PINGPONG_CWB_0,
		.base = 0x66000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_2,
	}, {
		.name = "pingpong_cwb_1", .id = PINGPONG_CWB_1,
		.base = 0x66400, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_2,
	}, {
		.name = "pingpong_cwb_2", .id = PINGPONG_CWB_2,
		.base = 0x7e000, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_3,
	}, {
		.name = "pingpong_cwb_3", .id = PINGPONG_CWB_3,
		.base = 0x7e400, .len = 0,
		.sblk = &sc7280_pp_sblk,
		.merge_3d = MERGE_3D_3,
	},
};

static const struct dpu_merge_3d_cfg tuna_merge_3d[] = {
	{
		.name = "merge_3d_0", .id = MERGE_3D_0,
		.base = 0x4e000, .len = 0x1c,
	}, {
		.name = "merge_3d_1", .id = MERGE_3D_1,
		.base = 0x4f000, .len = 0x1c,
	}, {
		.name = "merge_3d_2", .id = MERGE_3D_2,
		.base = 0x66700, .len = 0x1c,
	}, {
		.name = "merge_3d_3", .id = MERGE_3D_3,
		.base = 0x7e700, .len = 0x1c,
	},
};

/*
 * NOTE: Each display compression engine (DCE) contains dual hard
 * slice DSC encoders so both share same base address but with
 * its own different sub block address.
 *
 * M2582: DPU 12.3 is >= SDE_HW_VER_A00, so the factory driver sets
 * SDE_DSC_FULL_ICH_PREC on every one of these encoders
 * (sde_hw_catalog.c: "if (SDE_HW_MAJOR(sde_cfg->hw_rev) >=
 * SDE_HW_MAJOR(SDE_HW_VER_A00)) set_bit(SDE_DSC_FULL_ICH_PREC, ...)").
 * It is what makes ENC_DF_CTRL bit 12 get programmed for a 10-bit panel.
 */
static const struct dpu_dsc_cfg tuna_dsc[] = {
	{
		.name = "dce_0_0", .id = DSC_0,
		.base = 0x80000, .len = 0x8,
		.features = BIT(DPU_DSC_NATIVE_42x_EN) | BIT(DPU_DSC_FULL_ICH_PREC),
		.sblk = &sm8750_dsc_sblk_0,
	}, {
		.name = "dce_0_1", .id = DSC_1,
		.base = 0x80000, .len = 0x8,
		.features = BIT(DPU_DSC_NATIVE_42x_EN) | BIT(DPU_DSC_FULL_ICH_PREC),
		.sblk = &sm8750_dsc_sblk_1,
	}, {
		.name = "dce_1_0", .id = DSC_2,
		.base = 0x81000, .len = 0x8,
		.features = BIT(DPU_DSC_NATIVE_42x_EN) | BIT(DPU_DSC_FULL_ICH_PREC),
		.sblk = &sm8750_dsc_sblk_0,
	},
};

static const struct dpu_wb_cfg tuna_wb[] = {
	{
		.name = "wb_2", .id = WB_2,
		.base = 0x65000, .len = 0x2c8,
		.features = WB_SDM845_MASK,
		.format_list = wb2_formats_rgb_yuv,
		.num_formats = ARRAY_SIZE(wb2_formats_rgb_yuv),
		.xin_id = 6,
		.maxlinewidth = 4096,
		.intr_wb_done = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 4),
	},
};

static const struct dpu_cwb_cfg tuna_cwb[] = {
	{
		.name = "cwb_0", .id = CWB_0,
		.base = 0x66200, .len = 0x20,
	},
	{
		.name = "cwb_1", .id = CWB_1,
		.base = 0x66600, .len = 0x20,
	},
	{
		.name = "cwb_2", .id = CWB_2,
		.base = 0x7e200, .len = 0x20,
	},
	{
		.name = "cwb_3", .id = CWB_3,
		.base = 0x7e600, .len = 0x20,
	},
};

/*
 * Interface base map: sm8750's, which is also the vendor's.
 *
 * The DPU picks an interface by type and controller id, so a wrong base is
 * silent -- it drives a block nothing is wired to. The vendor's mmio window
 * starts at 0xae00000 and mainline's DPU node at 0xae01000, so every vendor
 * block offset is ours + 0x1000: our intf_1 = 0x35000 is the vendor's
 * 0x36000 (type "dsi"), and our tear block 0x35800 is theirs at 0x36800.
 * Every other block in this file is normalised the same way.
 */
static const struct dpu_intf_cfg tuna_intf[] = {
	{
		.name = "intf_0", .id = INTF_0,
		.base = 0x34000, .len = 0x4bc,
		.type = INTF_DP,
		.controller_id = MSM_DP_CONTROLLER_0,
		.prog_fetch_lines_worst_case = 24,
		.intr_underrun = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 24),
		.intr_vsync = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 25),
	}, {
		.name = "intf_1", .id = INTF_1,
		.base = 0x35000, .len = 0x4bc,
		.type = INTF_DSI,
		.controller_id = MSM_DSI_CONTROLLER_0,
		.prog_fetch_lines_worst_case = 24,
		.intr_underrun = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 26),
		.intr_vsync = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 27),
		/*
		 * Tear block is this base + 0x800: 0x35000 + 0x800 == 0x35800
		 * == MDP_INTF_REV_7xxx_TEAR_OFF(1). The vendor spells the same
		 * register 0x36800; see the note above tuna_intf[].
		 */
		.intr_tear_rd_ptr = DPU_IRQ_IDX(MDP_INTF1_TEAR_INTR, 2),
	}, {
		.name = "intf_2", .id = INTF_2,
		.base = 0x36000, .len = 0x4bc,
		.type = INTF_DSI,
		.controller_id = MSM_DSI_CONTROLLER_1,
		.prog_fetch_lines_worst_case = 24,
		.intr_underrun = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 28),
		.intr_vsync = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 29),
		.intr_tear_rd_ptr = DPU_IRQ_IDX(MDP_INTF2_TEAR_INTR, 2),
	}, {
		.name = "intf_3", .id = INTF_3,
		.base = 0x37000, .len = 0x4bc,
		.type = INTF_DP,
		.controller_id = MSM_DP_CONTROLLER_0,	/* pair with intf_0 for DP MST */
		.prog_fetch_lines_worst_case = 24,
		.intr_underrun = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 30),
		.intr_vsync = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 31),
	}
};

static const struct dpu_perf_cfg tuna_perf_data = {
	.max_bw_low = 23600000,
	.max_bw_high = 27800000,
	.min_core_ib = 2500000,
	.min_llcc_ib = 0,
	.min_dram_ib = 1600000,
	.min_prefill_lines = 35,
	.danger_lut_tbl = {0x3ffff, 0x3ffff, 0x0},
	.safe_lut_tbl = {0xfe00, 0xfe00, 0xffff},
	.qos_lut_tbl = {
		{.nentry = ARRAY_SIZE(sc7180_qos_linear),
		.entries = sc7180_qos_linear
		},
		{.nentry = ARRAY_SIZE(sc7180_qos_macrotile),
		.entries = sc7180_qos_macrotile
		},
		{.nentry = ARRAY_SIZE(sc7180_qos_nrt),
		.entries = sc7180_qos_nrt
		},
		/* TODO: macrotile-qseed is different from macrotile */
	},
	.cdp_cfg = {
		{.rd_enable = 1, .wr_enable = 1},
		{.rd_enable = 1, .wr_enable = 0}
	},
	.clk_inefficiency_factor = 105,
	.bw_inefficiency_factor = 120,
};

static const struct dpu_mdss_version tuna_mdss_ver = {
	.core_major_ver = 12,
	.core_minor_ver = 3,
};

const struct dpu_mdss_cfg dpu_tuna_cfg = {
	.mdss_ver = &tuna_mdss_ver,
	.caps = &tuna_dpu_caps,
	.mdp = &tuna_mdp,
	.cdm = &dpu_cdm_5_x,
	.ctl_count = ARRAY_SIZE(tuna_ctl),
	.ctl = tuna_ctl,
	.sspp_count = ARRAY_SIZE(tuna_sspp),
	.sspp = tuna_sspp,
	.mixer_count = ARRAY_SIZE(tuna_lm),
	.mixer = tuna_lm,
	.dspp_count = ARRAY_SIZE(tuna_dspp),
	.dspp = tuna_dspp,
	.pingpong_count = ARRAY_SIZE(tuna_pp),
	.pingpong = tuna_pp,
	.dsc_count = ARRAY_SIZE(tuna_dsc),
	.dsc = tuna_dsc,
	.merge_3d_count = ARRAY_SIZE(tuna_merge_3d),
	.merge_3d = tuna_merge_3d,
	.wb_count = ARRAY_SIZE(tuna_wb),
	.wb = tuna_wb,
	.cwb_count = ARRAY_SIZE(tuna_cwb),
	.cwb = tuna_cwb,
	.intf_count = ARRAY_SIZE(tuna_intf),
	.intf = tuna_intf,
	.vbif = &sm8650_vbif,
	.perf = &tuna_perf_data,
};

#endif
