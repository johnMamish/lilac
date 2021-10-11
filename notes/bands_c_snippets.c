static void compute_theta(struct band_ctx *ctx, struct split_ctx *sctx,
      celt_norm *X, celt_norm *Y, int N, int *b, int B, int B0,
      int LM,
      int stereo, int *fill)
{
    tell = ec_tell_frac(ec);

    qalloc = ec_tell_frac(ec) - tell;
    *b -= qalloc;
    sctx->qualloc = qualloc;
}


static unsigned quant_partition(struct band_ctx *ctx, celt_norm *X,
      int N, int b, int B, celt_norm *lowband,
      int LM,
      opus_val16 gain, int fill)
{
   if ((LM != -1) && (b > (max_unsplit_bits + 12)) && (N > 2))
   {
      compute_theta(ctx, &sctx, X, Y, N, &b, B, B0, LM, 0, &fill);
      qalloc = sctx.qalloc;

      ctx->remaining_bits -= qalloc;
      rebalance = ctx->remaining_bits;
      if (mbits >= sbits)
      {
         // RECURSIVE CALL
         cm = quant_partition(ctx, X, N, mbits, B, lowband, LM,
               MULT16_16_P15(gain,mid), fill);
         // rebalance - ctx->remaining_bits = # of bits used in previous quant_partition?
         rebalance = mbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=0)
            sbits += rebalance - (3<<BITRES);

         // RECURSIVE CALL
         cm |= quant_partition(ctx, Y, N, sbits, B, next_lowband2, LM,
               MULT16_16_P15(gain,side), fill>>B)<<(B0>>1);
      } else {
         // RECURSIVE CALL
         cm = quant_partition(ctx, Y, N, sbits, B, next_lowband2, LM,
               MULT16_16_P15(gain,side), fill>>B)<<(B0>>1);
         rebalance = sbits - (rebalance-ctx->remaining_bits);
         if (rebalance > 3<<BITRES && itheta!=16384)
            mbits += rebalance - (3<<BITRES);

         // RECURSIVE CALL
         cm |= quant_partition(ctx, X, N, mbits, B, lowband, LM,
               MULT16_16_P15(gain,mid), fill);
      }
   } else {
      /* This is the basic no-split case */
      curr_bits = pulses2bits(m, i, LM, q);
      ctx->remaining_bits -= curr_bits;

      /* Ensures we can never bust the budget */
      while (ctx->remaining_bits < 0 && q > 0) {
         ctx->remaining_bits += curr_bits;
         q--;
         curr_bits = pulses2bits(m, i, LM, q);
         ctx->remaining_bits -= curr_bits;
      }
   }
   return cm;
}


static unsigned quant_band(struct band_ctx *ctx, celt_norm *X,
      int N, int b, int B, celt_norm *lowband,
      int LM, celt_norm *lowband_out,
      opus_val16 gain, celt_norm *lowband_scratch, int fill)
{
   // Consume a codeword from ctx->ec and decode it into a vector, to be stored in X
   cm = quant_partition(ctx, X, N, b, B, lowband, LM, gain, fill);
}


void quant_all_bands(int encode, const CELTMode *m, int start, int end,
      celt_norm *X_, celt_norm *Y_, unsigned char *collapse_masks,
      const celt_ener *bandE, const int *pulses, int shortBlocks, int spread,
      int dual_stereo, int intensity, const int *tf_res, opus_int32 total_bits,
      opus_int32 balance, ec_ctx *ec, int LM, int codedBands,
      opus_uint32 *seed, int complexity, int arch, int disable_inv)
{
    opus_int32 remaining_bits;

    for (i=start;i<end;i++)
    {
        tell = ec_tell_frac(ec);

        /* Compute how many bits we want to allocate to this band */
        if (i != start)
            balance -= tell;
        remaining_bits = total_bits-tell-1;
        ctx.remaining_bits = remaining_bits;
        if (i <= codedBands-1) {
            curr_balance = celt_sudiv(balance, IMIN(3, codedBands-i));
            b = IMAX(0, IMIN(16383, IMIN(remaining_bits+1,pulses[i]+curr_balance)));
        } else {
            b = 0;
        }

        ////////////////////////////////////////////////
        // MONO PVQ DECODING HAPPENS HERE
        x_cm = quant_band(&ctx, X, N, b, B,
                          effective_lowband != -1 ? norm+effective_lowband : NULL, LM,
                          last?NULL:norm+M*eBands[i]-norm_offset, Q15ONE, lowband_scratch, x_cm|y_cm);

        balance += pulses[i] + tell;
    }
}
#endif
