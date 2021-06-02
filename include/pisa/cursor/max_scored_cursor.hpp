#pragma once

#include <vector>

#include "cursor/scored_cursor.hpp"
#include "query/queries.hpp"
#include "wand_data.hpp"

namespace pisa {

template <typename Cursor, typename Wand>
class MaxScoredCursor: public ScoredCursor<Cursor> {
  public:
    using base_cursor_type = Cursor;

    MaxScoredCursor(
            Cursor cursor, 
            TermScorer term_scorer, 
            float query_weight, 
            float max_score,
            typename Wand::wand_data_enumerator wdata)
        : ScoredCursor<Cursor>(std::move(cursor), std::move(term_scorer), query_weight),
          m_max_score(max_score), m_wdata(std::move(wdata))
    {}
    MaxScoredCursor(MaxScoredCursor const&) = delete;
    MaxScoredCursor(MaxScoredCursor&&) = default;
    MaxScoredCursor& operator=(MaxScoredCursor const&) = delete;
    MaxScoredCursor& operator=(MaxScoredCursor&&) = default;
    ~MaxScoredCursor() = default;

    [[nodiscard]] PISA_ALWAYSINLINE auto max_score() const noexcept -> float { return m_max_score; }

    void update_range_max_score(uint64_t range)
    {
        this->update_max_score(m_wdata.range_score(range));
    }

    float get_range_max_score(uint64_t range)
    {
      return m_wdata.range_score(range);
    }


  private:
    float m_max_score;
    float m_query_weight = 1.0;

    // ANYTIME: Allows us to update the local max score with the new per-bound score
  protected:
  
    // ANYTIME: Move wand data into max_scored_cursor so can access range max values
    typename Wand::wand_data_enumerator m_wdata;

    void update_max_score(float new_score) {
        m_max_score = this->query_weight() * new_score;
    }
};

template <typename Index, typename WandType, typename Scorer>
[[nodiscard]] auto
make_max_scored_cursors(Index const& index, WandType const& wdata, Scorer const& scorer, Query query)
{
    auto terms = query.terms;
    auto query_term_freqs = query_freqs(terms);

    std::vector<MaxScoredCursor<typename Index::document_enumerator, WandType>> cursors;
    cursors.reserve(query_term_freqs.size());
    std::transform(
        query_term_freqs.begin(), query_term_freqs.end(), std::back_inserter(cursors), [&](auto&& term) {
            float query_weight = term.second;
            auto max_weight = query_weight * wdata.max_term_weight(term.first);
            return MaxScoredCursor<typename Index::document_enumerator, WandType>(
                index[term.first], scorer.term_scorer(term.first), query_weight, max_weight, wdata.getenum(term.first));
        });
    return cursors;
}

}  // namespace pisa
